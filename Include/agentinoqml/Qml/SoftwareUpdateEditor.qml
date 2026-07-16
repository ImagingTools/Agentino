import QtQuick 2.0
import Acf 1.0
import com.imtcore.imtqml 1.0
import imtgui 1.0
import imtguigql 1.0
import imtdocgui 1.0
import imtcontrols 1.0
import agentinoUpdatesSdl 1.0

ViewBase {
	id: updateEditorContainer;

	property UpdateData updateData: model;
	property string clientId;

	Component.onCompleted: {
		tabPanel.addTab("General", qsTr("General"), mainEditorComp);
	}

	function updateGui(){
		let item = tabPanel.getTabByIndex(0);
		if (item){
			item.updateGui();
		}
	}

	function updateModel(){
		let item = tabPanel.getTabByIndex(0);
		if (item){
			item.updateModel();
		}
	}

	function getHeaders(){
		let headers = {}
		headers["clientid"] = clientId;
		return headers;
	}

	TabView {
		id: tabPanel
		anchors.fill: parent;
		currentIndex: 0;
	}

	Component {
		id: mainEditorComp;

		Flickable {
			id: flickable;

			anchors.left: parent.left;
			anchors.leftMargin: Style.marginXL;

			anchors.top: parent.top;
			anchors.topMargin: Style.marginXL;

			anchors.bottom: parent.bottom;
			anchors.bottomMargin: Style.marginXL;

			width: 600;
			contentHeight: contentColumn.height;
			clip: true;

			function updateGui(){}
			function updateModel(){}

			Column {
				id: contentColumn;
				width: parent.width;
				spacing: Style.marginM;

				FormField {
					label: qsTr("Name");
					TextInput {
						text: updateData ? updateData.m_name : ""
						readOnly: true
					}
				}

				FormField {
					label: qsTr("Version");
					TextInput {
						text: updateData ? updateData.m_version : ""
						readOnly: true
					}
				}

				FormField {
					label: qsTr("Type");
					TextInput {
						text: updateData ? (updateData.m_updateType === UpdateType.s_Agent ? qsTr("Agent") : qsTr("Service")) : ""
						readOnly: true
					}
				}

				FormField {
					label: qsTr("Description");
					TextInput {
						text: updateData ? updateData.m_description : ""
						readOnly: true
					}
				}

				FormField {
					label: qsTr("Published Date");
					TextInput {
						text: updateData ? updateData.m_publishedDate : ""
						readOnly: true
					}
				}

				FormField {
					label: qsTr("File Size");
					TextInput {
						text: updateData ? updateData.m_fileSize : ""
						readOnly: true
					}
				}

				FormField {
					label: qsTr("Checksum");
					TextInput {
						text: updateData ? updateData.m_checksum : ""
						readOnly: true
					}
				}

				FormField {
					label: qsTr("Status");
					TextInput {
						text: updateData ? updateData.m_status : ""
						readOnly: true
					}
				}

				FormField {
					label: qsTr("Changelog");
					TextInput {
						text: updateData ? updateData.m_changelog : ""
						readOnly: true
						wrapMode: Text.WordWrap
					}
				}

				Row {
					spacing: Style.marginM;

					Button {
						id: applyButton;
						text: qsTr("Apply Update");
						enabled: updateData && updateData.m_status !== UpdateStatus.s_Installed && updateData.m_status !== UpdateStatus.s_Installing
						onClicked: {
							applyUpdateRequestSender.send(applyUpdateInput)
						}
					}

					Button {
						id: rollbackButton;
						text: qsTr("Rollback");
						enabled: updateData && updateData.m_status === UpdateStatus.s_Installed
						onClicked: {
							rollbackUpdateRequestSender.send(rollbackUpdateInput)
						}
					}
				}
			}
		}
	}

	property ApplyUpdateInput applyUpdateInput: ApplyUpdateInput {
		m_updateId: updateData ? updateData.m_id : ""
		m_agentId: clientId
	}

	property RollbackUpdateInput rollbackUpdateInput: RollbackUpdateInput {
		m_updateId: updateData ? updateData.m_id : ""
		m_agentId: clientId
	}

	property GqlSdlRequestSender applyUpdateRequestSender: GqlSdlRequestSender {
		requestType: 1
		gqlCommandId: AgentinoUpdatesSdlCommandIds.s_applyUpdate

		sdlObjectComp: Component {
			ApplyUpdateResponse {
				onFinished: {
					updateEditorContainer.updateGui()
				}
			}
		}

		function getHeaders(){
			return updateEditorContainer.getHeaders()
		}
	}

	property GqlSdlRequestSender rollbackUpdateRequestSender: GqlSdlRequestSender {
		requestType: 1
		gqlCommandId: AgentinoUpdatesSdlCommandIds.s_rollbackUpdate

		sdlObjectComp: Component {
			RollbackUpdateResponse {
				onFinished: {
					updateEditorContainer.updateGui()
				}
			}
		}

		function getHeaders(){
			return updateEditorContainer.getHeaders()
		}
	}
}
