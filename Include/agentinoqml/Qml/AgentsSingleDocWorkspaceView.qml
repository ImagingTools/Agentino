import QtQuick 2.12
import Acf 1.0
import imtcontrols 1.0
import imtdocgui 1.0
import imtguigql 1.0
import imtgui 1.0
import agentinogqlSdl 1.0

SingleDocumentWorkspaceView {
    id: root;

    Component.onCompleted: {
        registerDocumentView("Agent", "AgentEditor", agentEditorComp);
        registerDocumentDataController("Agent", agentDataControllerComp);

        MainDocumentManager.registerDocumentManager("AgentsSingleDocument", root);

        addInitialItem(agentCollectionViewComp, "Agents");
    }

    Component {
        id: agentEditorComp;

        AgentEditor {
            id: agentEditor;

            commandsDelegateComp: Component {ViewCommandsDelegateBase {
                view: agentEditor;
            }
            }

            commandsControllerComp: Component {CommandsRepresentationProvider {
                commandId: "Agent";
                uuid: agentEditor.viewId;
            }
            }
        }
    }

    Component {
        id: agentCollectionViewComp;

        AgentCollectionView {}
    }

    Component {
        id: agentDataControllerComp

        GqlRequestDocumentDataController {
            id: requestDocumentDataController

            Component.onCompleted: {
                console.log("GqlRequestDocumentDataController onCompleted");
            }

             payloadModel: AgentDataPayload {
                 Component.onCompleted: {
                     console.log("AgentDataPayload onCompleted", this);
                 }
                 onM_itemChanged: {
                     console.log("AgentDataPayload onM_itemChanged", m_item);

                     m_item.connectProperties();
                     requestDocumentDataController.documentModel = m_item
                 }
             }

            gqlGetCommandId: "AgentItem";
            gqlUpdateCommandId: "AgentUpdate";
            gqlAddCommandId: "AgentAdd";
        }
    }
}
