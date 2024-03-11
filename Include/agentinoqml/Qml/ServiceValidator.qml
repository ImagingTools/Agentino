import QtQuick 2.15
import Acf 1.0
import imtdocgui 1.0
import imtauthgui 1.0

DocumentValidator {
    id: root;

    function isValid(data){
        let canChange = PermissionsController.checkPermission("ChangeService");
        if (!canChange){
            data.message = qsTr("Permission denied")

            return false;
        }

        return canChange;
    }
}


