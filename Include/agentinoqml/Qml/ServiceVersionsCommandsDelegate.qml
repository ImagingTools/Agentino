import QtQuick 2.12
import Acf 1.0
import com.imtcore.imtqml 1.0
import imtgui 1.0
import imtcolgui 1.0

// Commands of the versions page of a service (ServiceVersionsView.qml): Load, Remove, Install.
// Remove is the standard collection command (enabled with a selection, removes the selected versions);
// Install is enabled with a selection as well.
CollectionViewCommandsDelegateBase {
	id: container

	removeDialogTitle: qsTr("Deleting a service version")
	removeNameFieldId: "version"

	signal loadRequested()
	signal installRequested(var versionIds)

	function updateStateCustomCommands(selection, commandsController, elementsModel){
		commandsController.setCommandIsEnabled("Install", selection.length > 0)
	}

	onCommandActivated: {
		if (!container.collectionView || !container.collectionView.commandsController){
			return
		}

		if (!container.collectionView.commandsController.commandIsEnabled(commandId)){
			return
		}

		if (commandId === "Load"){
			container.loadRequested()
		}
		else if (commandId === "Install"){
			container.installRequested(container.collectionView.getSelectedIds())
		}
	}
}
