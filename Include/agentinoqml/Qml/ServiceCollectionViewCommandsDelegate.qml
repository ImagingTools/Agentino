import QtQuick 2.12
import Acf 1.0
import imtgui 1.0

CollectionViewCommandsDelegateBase {
    id: container;

    removeDialogTitle: qsTr("Deleting an agent");
    removeMessage: qsTr("Delete the selected agent ?");

    onSelectionChanged: {
        let indexes = container.tableData.getSelectedIndexes();
        let isEnabled = indexes.length > 0;

        if(container.commandsProvider){
            console.log("ServiceCollectionViewContainer onSelectionChanged", isEnabled);
            container.commandsProvider.setCommandIsEnabled("Start", isEnabled);
            container.commandsProvider.setCommandIsEnabled("Stop", isEnabled);
        }
    }

    onCommandActivated: {
        if (commandId === "Start" || commandId === "Stop"){
            console.log("ServiceCommands onCommandActivated", commandId);
            Events.sendEvent("ServiceCommandActivated", commandId);
        }
    }
}

