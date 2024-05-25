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

        width: parent.width
        height: 200 //container.height - log.height
        property string selectedServices

        onSelectionChanged: {
            if (selection.length > 0){
                let index = selection[0];
                log.agentId = agentCollectionView.table.elements.GetData("Id", index);
                selectedServices = agentCollectionView.table.elements.GetData("Services", index)
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
            dataController.elementsModel.Clear()
            if (dataController.collectionId !== log.collectionId){
                dataController.collectionId = log.collectionId
            }
            unRegisterSubscription()
            registerSubscription()
        }

        function getAdditionalInputParams(){
            let additionInputParams = {}
            additionInputParams["clientId"] = log.agentId;

            return additionInputParams
        }

        function handleSubscription(dataModel){
            if (!dataModel){
                return;
            }
            dataController.elementsModel.Clear()
            dataController.updateModel()

            // if (dataModel.ContainsKey("typeOperation")){
            //     let body = dataModel.GetData("typeOperation");
            //     if (body === "inserted"){
            //         dataController.elementsModel.Clear()
            //         dataController.updateModel()
            //     }
            // }
        }

        Component {
            id: messageCollectionFilterComp;

            MessageCollectionFilterDecorator {
                id: filterDecorator
                property string services: agentCollectionView.selectedServices

                onServicesChanged: {
                    checkMenu.dataModel.Clear()
                    var servicesModel = services.split(';')
                    for (let i = 0; i < servicesModel.length; i++){
                        checkMenu.dataModel.InsertNewItem()
                        checkMenu.dataModel.SetData("Name", servicesModel[i], i)
                    }
                    console.log("checkBoxMenu",checkMenu.dataModel)
                    filterDecorator.updateObjectFilter();
                }

                CheckBoxMenu{
                    id: checkMenu;

                    anchors.verticalCenter: parent.verticalCenter
                    anchors.left: filterDecorator.segmentedButton.right
                    anchors.leftMargin: Style.size_smallMargin
                    width: 200
                    height: 30
                    visible: filterDecorator.filtermenu.x - x < width ? false : true
                    placeHolderText: qsTr("Services");
                    menuHeight: delegateHeight  * (dataModel.GetItemsCount() + 1) ;
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
                    let objectFilter = log.collectionFilter.filterModel.GetData("ObjectFilter");
                    if (!objectFilter){
                        objectFilter = log.collectionFilter.filterModel.AddTreeModel("ObjectFilter")
                    }
                    let sourceFilter = objectFilter.GetData("Source");
                    if (!sourceFilter){
                        sourceFilter = objectFilter.AddTreeModel("Source")
                    }
                    for (let i = 0; i < checkMenu.dataModel.GetItemsCount(); i++){
                        let service = checkMenu.dataModel.GetData("Name", i)
                        let status = checkMenu.dataModel.GetData("checkState", i)
                        console.log("status", status)
                        sourceFilter.SetData(service, status === 0 || status === undefined ? false : true);
                    }

                    console.log("checkMenu objectFilter", objectFilter.ToJson())

                    log.dataController.updateModel()
                }
            }
        }
    }

}

