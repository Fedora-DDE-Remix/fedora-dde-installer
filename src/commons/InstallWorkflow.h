#ifndef FEDORA_DDE_INSTALL_WORKFLOW_H
#define FEDORA_DDE_INSTALL_WORKFLOW_H

// The live session owns one installation. Account setup is a second stage of
// that installation, never a first-boot service in the installed system.
class InstallWorkflow
{
public:
    enum class Stage {
        Selection,
        Installing,
        AwaitingUser,
        Configuring,
        Complete,
        InstallFailed,
        ConfigurationFailed
    };

    Stage stage() const { return m_stage; }
    bool beginInstall() { return change(Stage::Selection, Stage::Installing); }
    bool installationSucceeded() { return change(Stage::Installing, Stage::AwaitingUser); }
    bool installationFailed() { return change(Stage::Installing, Stage::InstallFailed); }
    bool beginConfiguration()
    {
        if (m_stage != Stage::AwaitingUser && m_stage != Stage::ConfigurationFailed)
            return false;
        m_stage = Stage::Configuring;
        return true;
    }
    bool configurationSucceeded() { return change(Stage::Configuring, Stage::Complete); }
    bool configurationFailed() { return change(Stage::Configuring, Stage::ConfigurationFailed); }
    bool canEditUser() const
    {
        return m_stage == Stage::AwaitingUser || m_stage == Stage::ConfigurationFailed;
    }
    bool canRestart() const { return m_stage == Stage::Complete; }
    bool canChangePartitions() const { return m_stage == Stage::Selection; }

private:
    bool change(Stage expected, Stage next)
    {
        if (m_stage != expected)
            return false;
        m_stage = next;
        return true;
    }
    Stage m_stage = Stage::Selection;
};

#endif
