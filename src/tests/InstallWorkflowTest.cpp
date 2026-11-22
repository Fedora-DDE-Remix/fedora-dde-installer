#include "InstallWorkflow.h"

#include <cstdlib>
#include <iostream>

static void require(bool condition, const char *message)
{
    if (!condition) {
        std::cerr << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

int main()
{
    InstallWorkflow flow;
    require(!flow.beginConfiguration(), "Cannot configure before installing a target");
    require(!flow.canRestart(), "Cannot restart before creating the user");
    require(flow.beginInstall(), "Installation must start from selection");
    require(!flow.beginInstall(), "Showing progress again must not reformat the disk");
    require(!flow.canChangePartitions(), "Partition navigation is locked once installation starts");
    require(flow.installationSucceeded(), "Successful installation enters account setup");
    require(flow.canEditUser(), "Account setup runs in the current live session");
    require(!flow.canRestart(), "Copy completion must never offer restart");
    require(!flow.configurationSucceeded(), "Cannot skip configuration");
    require(flow.beginConfiguration(), "Configuration may start after account entry");
    require(flow.configurationFailed(), "Configuration failure must be recoverable");
    require(flow.canEditUser(), "A failed configuration allows editing user details");
    require(!flow.canRestart(), "A failed configuration must not report success");
    require(!flow.beginInstall(), "Retrying configuration must never repeat partitioning");
    require(flow.beginConfiguration(), "Configuration can be retried");
    require(flow.configurationSucceeded(), "Finalization completes the installation");
    require(flow.canRestart(), "Only a configured, finalized target can be restarted");
    require(!flow.beginConfiguration(), "A completed installation cannot be configured twice");

    InstallWorkflow failed;
    require(failed.beginInstall() && failed.installationFailed(), "Installation error is recorded");
    require(!failed.canEditUser() && !failed.beginConfiguration(), "A failed copy cannot configure a target");
    require(!failed.canRestart(), "A failed installation cannot restart as a success");
    std::cout << "Install workflow regression checks passed\n";
}
