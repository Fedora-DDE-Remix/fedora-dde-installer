#!/bin/bash

setup_utcs() {
    local utcs_exec=$(installer_get DI_UTCS_EXEC_CONFIG)
    if program_exist $utcs_exec; then
        export DI_SYSROOT_DIR_ENV=/target
        export DI_PERSISTENT_DIR=/target/persistent

        local commitId sysCommitIdIdx
        commitId=$(ostree rev-parse --repo=$sysRootDir/ostree/repo $DI_REPO_BASE_NAME_ENV)
        sysCommitIdIdx="$commitId.0"
        export DI_COMMIT_ID_ENV=$sysCommitIdIdx

        local repo_dir=$DI_LIVE_DIR_ENV
        if [ ! -d "$repo_dir/dists" ]; then
            repo_dir=""
        fi
        export DI_LIVE_REPO_DIR_ENV=$repo_dir

        info "DI_SYSROOT_DIR_ENV=$DI_SYSROOT_DIR_ENV"
        info "DI_PERSISTENT_DIR=$DI_PERSISTENT_DIR"
        info "DI_COMMIT_ID_ENV=$DI_COMMIT_ID_ENV"
        info "DI_LIVE_REPO_DIR_ENV=$DI_LIVE_REPO_DIR_ENV"

        info "exec ${utcs_exec} --installer-target"

        $utcs_exec --installer-target || warning "exec ${utcs_exec} --installer-target failed"
    else
        warning "the $utcs_exec tool module could not be found"
    fi
}

install_nvidia_driver() {
    if lspci -n | grep -i 10de | egrep -i '.*03(80|0[0-2])'; then
        debconf-set-selections <<< 'nvidia-legacy-check nvidia-driver/check-for-unsupported-gpu select Yes'
        debconf-set-selections <<< 'nvidia-legacy-check nvidia-driver/install-even-if-unsupported-gpu-exists select OK'
        install_package nvidia-driver || info "install nvidia driver failed!"
    else
        warning "nvidia device could not be found"
    fi
}