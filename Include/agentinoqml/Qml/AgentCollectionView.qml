import QtQuick 2.12
import Acf 1.0
import imtcontrols 1.0
import imtcolgui 1.0
import imtguigql 1.0
import imtdocgui 1.0
import imtgui 1.0

SplitView {
    id: container

    anchors.fill: parent
    hasAnimation: true;

    orientation: Qt.Vertical

    AgentCollectionViewBase {
        id: agentCollectionView;

        height: 200
        property string selectedServices

        onSelectionChanged: {
            if (selection.length > 0){
                let index = selection[0];
                log.agentId = agentCollectionView.table.elements.getData("id", index);
                selectedServices = agentCollectionView.table.elements.getData("services", index)
            }
            else{
                log.agentId = ""
            }
        }
    }

    MessageCollectionView {
        id: log

        property string agentId

        collectionId: "AgentLog";

        collectionFilter: MessageCollectionFilter {
        }

        Component.onCompleted: {
            filterMenu.decorator = messageCollectionFilterComp;
        }

        onAgentIdChanged: {
            console.log("onAgentIdChanged", agentId);
            dataController.elementsModel.clear()
            if (dataController.collectionId !== log.collectionId){
                dataController.collectionId = log.collectionId
            }
            console.log("onAgentIdChanged collectionId", dataController.collectionId);

			// unRegisterSubscription()
			// registerSubscription()
        }

        function getHeaders(){
            let additionInputParams = {}
            additionInputParams["clientid"] = log.agentId;

            return additionInputParams
        }

        function handleSubscription(dataModel){
            if (!dataModel){
                return;
            }
            dataController.elementsModel.clear()
            dataController.updateModel()
        }

        Component {
            id: messageCollectionFilterComp;

            MessageCollectionFilterDecorator {
                id: filterDecorator
                property string services: agentCollectionView.selectedServices

                onServicesChanged: {
                    if (services != ""){
                        checkMenu.dataModel.clear()
                        var servicesModel = services.split(';')
                        for (let i = 0; i < servicesModel.length; i++){
                            checkMenu.dataModel.insertNewItem()
                            checkMenu.dataModel.setData("Name", servicesModel[i], i)
                        }
                        // filterDecorator.updateObjectFilter();
                    }
                }

                CheckBoxMenu{
                    id: checkMenu;

                    anchors.verticalCenter: parent.verticalCenter
                    anchors.left: filterDecorator.segmentedButton.right
                    anchors.leftMargin: Style.sizeSmallMargin
                    width: 200
                    height: 30
                    visible: filterDecorator.filtermenu.x - x < width ? false : true
                    placeHolderText: qsTr("Services");
                    menuHeight: delegateHeight  * (dataModel.getItemsCount() + 1) ;
                    delegateHeight: 40;
                    hasSearch: false;
                    canOpenMenu: true;
                    nameId: "Name";
                    onMenuCreated: {
                    }
                    onChangedSignal: {
                        filterDecorator.updateObjectFilter();
                    }
                }

                function updateObjectFilter(){
                    let objectFilter = log.collectionFilter.filterModel.getData("ObjectFilter");
                    if (!objectFilter){
                        objectFilter = log.collectionFilter.filterModel.addTreeModel("ObjectFilter")
                    }
                    let sourceFilter = objectFilter.getData("Source");
                    if (!sourceFilter){
                        sourceFilter = objectFilter.addTreeModel("Source")
                    }
                    for (let i = 0; i < checkMenu.dataModel.getItemsCount(); i++){
                        let service = checkMenu.dataModel.getData("Name", i)
                        let status = checkMenu.dataModel.getData("checkState", i)
                        console.log("status", status)
                        sourceFilter.setData(service, status === 0 || status === undefined ? false : true);
                    }

                    log.doUpdateGui()
                }
            }
        }
    }

}

