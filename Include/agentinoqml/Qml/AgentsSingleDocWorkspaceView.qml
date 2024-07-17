import QtQuick 2.12
import Acf 1.0
import imtcontrols 1.0
import imtdocgui 1.0
import imtguigql 1.0
import imtgui 1.0
import agentinogqlSdl 1.0

SingleDocumentWorkspaceView {
    id: root;

    anchors.fill: parent;

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

            commandsControllerComp: Component {CommandsPanelController {
                commandId: "Agent";
                uuid: agentEditor.viewId;
            }
            }
        }
    }

    Component {
        id: agentCollectionViewComp;

        AgentCollectionView {
            anchors.fill: root;
        }
    }

    Component {
        id: agentDataControllerComp

        GqlRequestDocumentDataController {
            id: requestDocumentDataController

            documentModelComp: Component {
                AgentData {
                    Component.onCompleted: {
                        console.log("AgentData onCompleted");
                    }
                }
            }

             payloadModel: AgentDataPayload {
                 onM_itemChanged: {
                     requestDocumentDataController.documentModel = m_item
                 }
             }

            gqlGetCommandId: "AgentItem";
            gqlUpdateCommandId: "AgentUpdate";
            gqlAddCommandId: "AgentAdd";
        }
    }
}
