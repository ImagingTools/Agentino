import QtQuick 2.12
import Acf 1.0
import com.imtcore.imtqml 1.0
import imtgui 1.0
import imtcolgui 1.0

// Commands of the versions page of a service (ServiceVersionsView.qml): Add, Remove, Install, Set service folder.
// Add opens the choice of the folder of a version inside the folder of the service versions (enabled
// when the page shows the versions of one operating system family and that folder is set - addEnabled);
// Remove is the standard collection command (enabled with a selection, removes the selected versions);
// Install is enabled with one selected version: it opens the choice of the agents to install it on;
// Set service folder opens the choice of the folder of the service versions for the operating system
// family of the page (enabled when the page shows one family - setServiceFolderEnabled).
CollectionViewCommandsDelegateBase {
	id: container

	removeDialogTitle: qsTr("Deleting a service version")
	removeNameFieldId: "version"

	// A version can be added: one operating system family, the folder of its versions is set.
	property bool addEnabled: true
	// The folder of the versions can be set: the page shows the versions of one operating system family.
	property bool setServiceFolderEnabled: true

	signal addRequested()
	signal installRequested(var versionIds)
	signal setServiceFolderRequested()

	function updatePageCommands(){
		if (container.collectionView && container.collectionView.commandsController){
			container.collectionView.commandsController.setCommandIsEnabled("Add", container.addEnabled)
			container.collectionView.commandsController.setCommandIsEnabled("SetServiceFolder", container.setServiceFolderEnabled)
		}
	}

	function updateStateCustomCommands(selection, commandsController, elementsModel){
		commandsController.setCommandIsEnabled("Add", container.addEnabled)
		commandsController.setCommandIsEnabled("SetServiceFolder", container.setServiceFolderEnabled)
		// One version of a service is installed at once.
		commandsController.setCommandIsEnabled("Install", selection.length === 1)
	}

	onAddEnabledChanged: {
		container.updatePageCommands()
	}

	onSetServiceFolderEnabledChanged: {
		container.updatePageCommands()
	}

	// Add and Set service folder come from the server disabled; a change made before the commands
	// arrive is applied on their arrival (CommandsController.pendingChanges).
	Component.onCompleted: {
		container.updatePageCommands()
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
		else if (commandId === "SetServiceFolder"){
			container.setServiceFolderRequested()
		}
	}
}
