import QtQuick 2.12
import Acf 1.0
import com.imtcore.imtqml 1.0
import imtcontrols 1.0
import imtdocgui 1.0
import imtguigql 1.0
import imtgui 1.0
import agentinoAgentsSdl 1.0

SingleDocumentWorkspaceView {
	id: root;
	
	anchors.fill: parent;
	documentManager: DocumentManager {}
	visualStatusProvider: GqlBasedObjectVisualStatusProvider {}
	
	Component.onCompleted: {
		documentManager.registerDocumentView("Agent", "AgentEditor", agentEditorComp);
		documentManager.registerDocumentDataController("Agent", agentDataControllerComp);
		
		MainDocumentManager.registerDocumentManager("AgentsSingleDocument", documentManager);
		
		addInitialItem(agentCollectionViewComp, "Agents");
	}
	
	Component {
		id: agentEditorComp;
		
		AgentEditor {
			id: agentEditor;
			
			commandsControllerComp: Component {GqlBasedCommandsController {
					typeId: "Agent";
				}
			}
		}
	}
	
	Component {
		id: agentCollectionViewComp;
		
		AgentCollectionView {
			width: root.width;
			height: root.height;
		}
	}
	
	Component {
		id: agentDataControllerComp
		
		GqlRequestDocumentDataController {
			id: requestDocumentDataController
			
			documentModelComp: Component {
				AgentData {}
			}
			
			property AgentData agentData: documentModel;
			
			// typeId: "Agent";
			documentName: agentData ? agentData.m_name: "";
			documentDescription: agentData ? agentData.m_description: "";
			
			gqlGetCommandId: AgentinoAgentsSdlCommandIds.s_getAgent;
			gqlUpdateCommandId: AgentinoAgentsSdlCommandIds.s_updateAgent;
			gqlAddCommandId: AgentinoAgentsSdlCommandIds.s_addAgent;
		}
	}
}
