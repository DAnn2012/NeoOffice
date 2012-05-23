<script>
<![CDATA[

function runBashScriptAndSetResult(bashScript, volume) {
    if (bashScript == null)
        return true;

    var procResult;
    if (volume != null) {
        procResult = system.run('/bin/bash', '-x', '-c', unescape(bashScript), '/bin/bash', volume);
    }
    else {
        procResult = system.run('/bin/bash', '-x', '-c', unescape(bashScript), '/bin/bash');
    }

    if (procResult === 0) {
        return true;
    }
    else {
        my.result.title = '$(PRODUCT_NAME_AND_VERISON)';
        my.result.type = 'Fatal';
        my.result.message = '';

		var message = null;
        if (!isNaN(procResult)) {
            // Temporary workaround to InstallationCheck exit values
            if (procResult > 96) {
                procResult -= 96;
            }

            var key = procResult.toString();
            message = system.localizedString(key);
            if (message == key) {
                message = null;
            }
        }

        if (message == null) {
            if (volume != null) {
                message = system.localizedStandardString('GENERIC_FAIL_VOLUME');
            }
            else {
                message = system.localizedStandardStringWithFormat('InstallationCheckError', '$(PRODUCT_NAME_AND_VERISON)');
            }
        }

        if (message != null) {
            my.result.message = message.toString();
        }

        return false;
    }
}

function runInstallationCheck() {
    // Script must an escaped string with no newlines
    var installationCheckBashScript = null;
    return runBashScriptAndSetResult(installationCheckBashScript, null);
}

function runVolumeCheck() {
    // Script must an escaped string with no newlines
    var volumeCheckBashScript = null;
    return runBashScriptAndSetResult(volumeCheckBashScript, my.target.mountpoint);
}

]]>
</script>

<installation-check script="runInstallationCheck();"/>

<volume-check script="runVolumeCheck();"/>
