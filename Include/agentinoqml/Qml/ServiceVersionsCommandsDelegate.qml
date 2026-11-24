import QtQuick 2.12
import Acf 1.0
import com.imtcore.imtqml 1.0
import imtgui 1.0
import imtcolgui 1.0

// Commands of the versions page of a service (ServiceVersionsView.qml): Add, Remove, Install.
// Add opens the choice of the folder of a version on the server (enabled when the page shows the
// versions of one operating system family - addEnabled);
// Remove is the standard collection command (enabled with a selection, removes the selected versions);
// Install is enabled with a selection as well.
CollectionViewCommandsDelegateBase {
	id: container

	removeDialogTitle: qsTr("Deleting a service version")
	removeNameFieldId: "version"

	// A version can be added: the page shows the versions of one operating system family.
	property bool addEnabled: true

	signal addRequested()
	signal installRequested(var versionIds)

	function updateAddCommand(){
		if (container.collectionView && container.collectionView.commandsController){
			container.collectionView.commandsController.setCommandIsEnabled("Add", container.addEnabled)
		}
	}

	function updateStateCustomCommands(selection, commandsController, elementsModel){
		commandsController.setCommandIsEnabled("Add", container.addEnabled)
		commandsController.setCommandIsEnabled("Install", selection.length > 0)
	}

	onAddEnabledChanged: {
		container.updateAddCommand()
	}

	// Add comes from the server enabled; a change made before the commands arrive is applied on their
	// arrival (CommandsController.pendingChanges).
	Component.onCompleted: {
		container.updateAddCommand()
	}

	onCommandActivated: {
		if (!container.collectionView || !container.collectionView.commandsController){
			return
		}

		if (!container.collectionView.commandsController.commandIsEnabled(commandId)){
			return
		}

		if (commandId === "Add"){
			container.addRequested()
		}
		else if (commandId === "Install"){
			container.installRequested(container.collectionView.getSelectedIds())
		}
	}
}
