import QtQuick 2.12
import Acf 1.0
import com.imtcore.imtqml 1.0
import imtcontrols 1.0
import imtdocgui 1.0
import imtguigql 1.0
import agentinoAgentsSdl 1.0

// The agent log used to live here as a second SplitView pane (scoped to the selected row's
// id, with a services-name filter) - it is now the "Log" page inside AgentEditor, alongside
// "Services" (also moved out of here), so this is just the collection grid.
//
// This is the AgentsPage's FixedItemQmlPath component (see Pages.acc), so it is the pinned
// home tab inside the page's own MultiDocWorkspaceView - opening an agent now opens a real
// separate tab (MultiDoc) instead of swapping this grid out in place, by analogy with how
// TopologyPage.qml registers "Service" against MainDocumentService.getDocumentService("Topology")
// rather than a private SingleDocumentWorkspaceView.
AgentCollectionViewBase {
	id: container

	anchors.fill: parent

	property var documentManager: MainDocumentService.getDocumentService("Agents")

	Component.onCompleted: {
		if (container.documentManager){
			container.documentManager.registerDocumentView("Agent", agentEditorComp);
			container.documentManager.registerDocumentDataController("Agent", agentDataControllerComp);
		}
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
		id: agentDataControllerComp

		GqlRequestDocumentDataController {
			id: requestDocumentDataController

			documentModelComp: Component {
				AgentData {}
			}

			property AgentData agentData: documentModel;

			documentName: agentData ? agentData.m_name: "";
			documentDescription: agentData ? agentData.m_description: "";

			gqlGetCommandId: AgentinoAgentsSdlCommandIds.s_getAgent;
			gqlUpdateCommandId: AgentinoAgentsSdlCommandIds.s_updateAgent;
			gqlAddCommandId: AgentinoAgentsSdlCommandIds.s_addAgent;
		}
	}
}
