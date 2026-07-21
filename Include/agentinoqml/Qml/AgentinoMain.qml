import QtQuick 2.0
import Acf 1.0
import com.imtcore.imtqml 1.0
import imtgui 1.0
import imtcontrols 1.0
import imtauthgui 1.0

ApplicationMain{
    id: window;
    useWebSocketSubscription: true
    canRecoveryPassword: false;
    authorizationServerConnected: true;

    // Composition root (Architecture §6.5) — shells inject adapters/scope here.
    property var agentinoBackend: AgentinoBackend {}
}

