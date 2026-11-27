#!/usr/bin/python3
"""Real Linux mount isolation regression, for the isolated CI host test only.

Run as: sudo unshare --mount --propagation private -- python3 -I this_file
Only temporary tmpfs/bind mounts and test-owned processes are used. No block
device, real /target, disk utility, lazy unmount, or host service is involved.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import select
import shutil
import stat
import subprocess
import sys
import tempfile
import time


SCRIPT = Path(__file__).resolve()
EXECUTOR = SCRIPT.parents[2] / "tools/scripts/fedora_namespace.py"
PYTHON = str(Path(sys.executable).resolve())
DEADLINE = 20


def require(condition, message):
    if not condition:
        raise RuntimeError(message)


def run(*arguments, **kwargs):
    result = subprocess.run(arguments, text=True, stdout=subprocess.PIPE,
                            stderr=subprocess.PIPE, timeout=DEADLINE, **kwargs)
    require(result.returncode == 0,
            f"{arguments[0]} exited {result.returncode}: {result.stderr.strip()}")
    return result.stdout


def identity():
    value = Path("/proc/self/ns/mnt").stat()
    return {"device": value.st_dev, "inode": value.st_ino}


def verified_identity():
    # The real production verifier must accept every separately spawned stage.
    return json.loads(run(PYTHON, "-I", str(EXECUTOR), "--check"))


def mounts():
    def decode(value):
        return re.sub(r"\\([0-7]{3})", lambda match: chr(int(match[1], 8)), value)

    result = []
    for line in Path("/proc/self/mountinfo").read_text().splitlines():
        before, separator, after = line.partition(" - ")
        fields, filesystem = before.split(), after.split()
        require(separator and len(fields) >= 6 and len(filesystem) >= 3,
                "Malformed kernel mount information")
        result.append({"id": int(fields[0]), "parent": int(fields[1]),
                       "device": fields[2], "root": decode(fields[3]),
                       "path": decode(fields[4]), "propagation": fields[6:],
                       "fstype": filesystem[0], "source": decode(filesystem[1])})
    return result


def below(path, root):
    return path == str(root) or path.startswith(str(root) + "/")


def target_mounts(root):
    return [item for item in mounts()
            if below(item["path"], root / "target")
            or below(item["path"], root / "state/btrfs-top-level")]


def snapshot(root):
    return {"pid": os.getpid(), "namespace": identity(),
            "target_mounts": target_mounts(root),
            "sandbox_mounts": [item for item in mounts() if item["path"] == str(root)],
            "target_marker_visible": (root / "target/installation-marker").exists()}


def mount_tmpfs(path, source, size="4m"):
    path.mkdir(mode=0o755, parents=True, exist_ok=True)
    run("/usr/bin/mount", "--types", "tmpfs", "--options",
        f"size={size},mode=0755,nosuid,nodev,noexec", source, str(path))


def unmount_targets(root):
    # Each ordinary unmount must succeed. Never fall back to lazy/force cleanup.
    for item in sorted(target_mounts(root), key=lambda value: value["path"].count("/"), reverse=True):
        run("/usr/bin/umount", item["path"])
    require(not target_mounts(root), "Installation mounts survived ordinary unmount")


def stage(operation, root):
    require(os.geteuid() == 0, "The integration stage requires root")
    require(root.parent == Path("/tmp") and root.name.startswith("fedora-dde-mount-ns-")
            and root.resolve() == root and root.stat().st_uid == 0,
            "The integration stage only accepts its own temporary sandbox")
    current = verified_identity()
    if operation == "mount":
        require(not target_mounts(root), "The installation fixture is already mounted")
        mount_tmpfs(root / "state/btrfs-top-level", "installer-preparation")
        mount_tmpfs(root / "target", "installer-target", "16m")
        (root / "target/installation-marker").write_text("temporary integration data\n")
        mount_tmpfs(root / "target/boot", "installer-boot")
        for name in ("dev", "sys"):
            destination = root / "target" / name
            destination.mkdir()
            run("/usr/bin/mount", "--rbind", str(root / "runtime-source" / name), str(destination))
            run("/usr/bin/mount", "--make-rslave", str(destination))
        mount_tmpfs(root / "target/run", "installer-runtime")
    elif operation == "cleanup":
        unmount_targets(root)
    elif operation != "inspect":
        raise RuntimeError(f"Unknown integration stage: {operation}")
    result = snapshot(root)
    require(result["namespace"] == current == verified_identity(),
            "An installation stage changed mount namespace")
    result["stage"] = operation
    result["all_mounted_devices"] = sorted({item["device"] for item in mounts()})
    return result


def serve(role, root):
    initial = snapshot(root)
    if role == "worker":
        require(initial["namespace"] == verified_identity(), "Executor did not preserve its namespace")
    print(json.dumps(initial), flush=True)
    for line in sys.stdin:
        operation = json.loads(line)["operation"]
        if operation == "quit":
            print(json.dumps({"stopped": True}), flush=True)
            return
        if role == "observer":
            require(operation == "inspect", "Observers cannot mutate mounts")
            result = snapshot(root)
        else:
            result = json.loads(run(PYTHON, "-I", str(SCRIPT), "--stage", operation, str(root)))
        print(json.dumps(result), flush=True)


class Peer:
    """A bounded JSON pipe to a test-owned long-lived process."""

    def __init__(self, arguments):
        self.errors = tempfile.TemporaryFile(mode="w+t")
        self.process = subprocess.Popen(arguments, stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                                        stderr=self.errors)
        self.buffer = b""
        try:
            self.initial = self.receive()
        except BaseException:
            self.stop()
            raise

    def receive(self):
        expires = time.monotonic() + DEADLINE
        while b"\n" not in self.buffer:
            remaining = expires - time.monotonic()
            require(remaining > 0 and select.select([self.process.stdout], [], [], max(remaining, 0))[0],
                    "A test process exceeded its response deadline")
            chunk = os.read(self.process.stdout.fileno(), 65536)
            if not chunk:
                self.errors.seek(0)
                raise RuntimeError("A test process exited without a response: " + self.errors.read()[-4000:])
            self.buffer += chunk
            require(len(self.buffer) <= 1024 * 1024, "A test response exceeded its output budget")
        line, self.buffer = self.buffer.split(b"\n", 1)
        return json.loads(line)

    def request(self, operation):
        self.process.stdin.write((json.dumps({"operation": operation}) + "\n").encode())
        self.process.stdin.flush()
        return self.receive()

    def stop(self):
        clean = True
        try:
            if self.process.poll() is None:
                self.request("quit")
            self.process.wait(timeout=5)
            clean = self.process.returncode == 0
        except (OSError, RuntimeError, subprocess.TimeoutExpired):
            clean = False
            self.process.terminate()
            try:
                self.process.wait(timeout=5)
            except subprocess.TimeoutExpired:
                self.process.kill()
                self.process.wait(timeout=5)
        finally:
            self.errors.close()
        return clean


def exercise(report):
    require(os.geteuid() == 0, "Run this integration test inside sudo unshare --mount --propagation private")
    # This gate is before any mutation: never modify the real CI host namespace.
    report["outer_namespace"] = verified_identity()
    initial_mountinfo = Path("/proc/self/mountinfo").read_text()
    root = Path(tempfile.mkdtemp(prefix="fedora-dde-mount-ns-", dir="/tmp"))
    root.chmod(0o755)
    peers = []
    worker = None
    try:
        mount_tmpfs(root, "installer-namespace-test", "32m")
        (root / "target").mkdir()
        (root / "state").mkdir(mode=0o700)
        for name in ("dev", "sys", "dev/pts"):
            mount_tmpfs(root / "runtime-source" / name, "fixture-" + name.replace("/", "-"))
        run("/usr/bin/mount", "--make-rshared", str(root))

        # Prove the shared fixture and observers really detect mount propagation.
        observer = Peer(["/usr/bin/unshare", "--mount", "--propagation", "slave", "--",
                         PYTHON, "-I", str(SCRIPT), "--observer", str(root)])
        peers.append(observer)
        require(any(tag.startswith("master:") for item in observer.initial["sandbox_mounts"]
                    for tag in item["propagation"]), "The existing observer is not a slave of the shared fixture")
        mount_tmpfs(root / "target", "baseline-shared-target")
        report["shared_positive_control"] = observer.request("inspect")
        require(len(report["shared_positive_control"]["target_mounts"]) == 1,
                "The positive control did not propagate to the existing service namespace")
        run("/usr/bin/umount", str(root / "target"))
        require(not observer.request("inspect")["target_mounts"], "Positive-control unmount did not propagate")

        rejected = subprocess.run([PYTHON, "-I", str(EXECUTOR), "--check"], text=True,
                                  stdout=subprocess.PIPE, stderr=subprocess.PIPE, timeout=DEADLINE)
        require(rejected.returncode != 0 and "recursively private" in rejected.stderr,
                "The production verifier accepted the shared host-like fixture")
        report["shared_namespace_rejected"] = True

        # A GitHub checkout may be below a non-traversable runner home. Recreate
        # the installed root-owned/readable code permissions inside our tmpfs;
        # never change permissions on the runner's checkout or home directory.
        code = root / "unprivileged-code"
        code.mkdir(mode=0o755)
        code.chmod(0o755)
        require(code.stat().st_uid == 0 and stat.S_IMODE(code.stat().st_mode) == 0o755,
                "The non-root test code directory is not root-owned and readable")
        report["unprivileged_code_inputs"] = []
        for source in (EXECUTOR, EXECUTOR.with_name("fedora_backend.py")):
            copied = code / source.name
            content = source.read_bytes()
            copied.write_bytes(content)
            copied.chmod(0o644)
            original_hash = hashlib.sha256(content).hexdigest()
            copied_hash = hashlib.sha256(copied.read_bytes()).hexdigest()
            require(original_hash == copied_hash and copied.stat().st_uid == 0
                    and stat.S_IMODE(copied.stat().st_mode) == 0o644,
                    "The non-root test code differs from the production source")
            report["unprivileged_code_inputs"].append({"name": source.name,
                                                       "source_sha256": original_hash,
                                                       "copy_sha256": copied_hash,
                                                       "owner_uid": 0, "mode": "0644"})
        public = root / "unprivileged-output"
        public.mkdir(mode=0o777)
        public.chmod(0o777)
        marker = public / "unexpected-execution"
        rejected = subprocess.run([PYTHON, "-I", str(code / EXECUTOR.name), "--exec", "/usr/bin/touch", str(marker)],
                                  user=65534, group=65534, extra_groups=[], cwd=public, text=True,
                                  stdout=subprocess.PIPE, stderr=subprocess.PIPE, timeout=DEADLINE)
        report["unprivileged_exec"] = {"uid": 65534, "returncode": rejected.returncode,
                                       "stderr": rejected.stderr[-4000:],
                                       "stdout": rejected.stdout[-4000:],
                                       "marker_exists": marker.exists()}
        require(rejected.returncode != 0 and "requires root privileges" in rejected.stderr
                and not marker.exists(), "An unprivileged executor request was not rejected before execution")
        report["unprivileged_exec_rejected"] = True

        worker = Peer([PYTHON, "-I", str(EXECUTOR), "--exec", PYTHON, "-I",
                       str(SCRIPT), "--worker", str(root)])
        peers.append(worker)
        expected = worker.initial["namespace"]
        require(expected != report["outer_namespace"] and expected != observer.initial["namespace"],
                "The production executor did not create a separate mount namespace")
        mounted = worker.request("mount")
        inspected = worker.request("inspect")
        report["installation_stages"] = [mounted, inspected]
        require(mounted["namespace"] == inspected["namespace"] == expected
                and mounted["pid"] != inspected["pid"], "Independent installation stages did not retain their namespace")
        require(inspected["target_marker_visible"] and len(inspected["target_mounts"]) == 7,
                "The subsequent stage cannot see all target, preparation and runtime mounts")

        report["existing_observer"] = observer.request("inspect")
        report["host_after_mount"] = snapshot(root)
        # This clone is deliberately created by the host-like parent after mount.
        late = Peer(["/usr/bin/unshare", "--mount", "--propagation", "slave", "--",
                     PYTHON, "-I", str(SCRIPT), "--observer", str(root)])
        peers.append(late)
        report["late_observer"] = late.request("inspect")
        for name in ("existing_observer", "host_after_mount", "late_observer"):
            require(not report[name]["target_mounts"] and not report[name]["target_marker_visible"],
                    f"Installation mounts leaked into {name}")

        devices = {item["device"] for item in mounted["target_mounts"]
                   if item["path"] in {str(root / "target"), str(root / "state/btrfs-top-level")}}
        cleaned = worker.request("cleanup")
        final = worker.request("inspect")
        report["installation_stages"].extend([cleaned, final])
        require(cleaned["namespace"] == final["namespace"] == expected
                and not final["target_mounts"] and not final["target_marker_visible"]
                and devices.isdisjoint(final["all_mounted_devices"]),
                "Ordinary cleanup left an installation filesystem mounted")
        for peer in (observer, late):
            require(not peer.request("inspect")["target_mounts"], "An external namespace retained installation mounts")
        report["ordinary_cleanup_verified"] = True
    finally:
        if worker is not None and worker.process.poll() is None:
            # Also try ordinary cleanup after a failed assertion, before exiting
            # this test-owned namespace; cleanup failure can never become PASS.
            try:
                worker.request("cleanup")
            except (OSError, RuntimeError, subprocess.TimeoutExpired) as error:
                report.setdefault("cleanup_errors", []).append(str(error))
        for peer in reversed(peers):
            if not peer.stop():
                report.setdefault("cleanup_errors", []).append("A test-owned process did not stop normally")
        for item in sorted([item for item in mounts() if below(item["path"], root)],
                           key=lambda value: value["path"].count("/"), reverse=True):
            try:
                run("/usr/bin/umount", item["path"])
            except (RuntimeError, subprocess.TimeoutExpired) as error:
                report.setdefault("cleanup_errors", []).append(str(error))
        remaining = [item for item in mounts() if below(item["path"], root)]
        if not remaining:
            shutil.rmtree(root)
        require(not remaining and not report.get("cleanup_errors"), "Integration resources were not cleaned normally")
        require(Path("/proc/self/mountinfo").read_text() == initial_mountinfo,
                "The isolated host-like mount tree did not return to its initial state")
        report["outer_mount_tree_restored"] = True


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    modes = parser.add_mutually_exclusive_group()
    modes.add_argument("--observer", type=Path)
    modes.add_argument("--worker", type=Path)
    modes.add_argument("--stage", choices=("mount", "inspect", "cleanup"))
    parser.add_argument("root", type=Path, nargs="?")
    parser.add_argument("--report", type=Path)
    arguments = parser.parse_args()
    if arguments.observer or arguments.worker:
        serve("observer" if arguments.observer else "worker", arguments.observer or arguments.worker)
        return 0
    if arguments.stage:
        require(arguments.root is not None, "A stage requires its sandbox path")
        print(json.dumps(stage(arguments.stage, arguments.root)), flush=True)
        return 0
    report = {"schema": 1, "test": "installer-mount-namespace", "passed": False}
    try:
        exercise(report)
        report["passed"] = True
    except (OSError, RuntimeError, subprocess.TimeoutExpired) as error:
        report["error"] = str(error)
    if arguments.report:
        arguments.report.parent.mkdir(parents=True, exist_ok=True)
        arguments.report.write_text(json.dumps(report, indent=2, sort_keys=True) + "\n")
    print("MOUNT_NAMESPACE_INTEGRATION " + json.dumps(report, sort_keys=True), flush=True)
    return 0 if report["passed"] else 1


if __name__ == "__main__":
    sys.exit(main())
