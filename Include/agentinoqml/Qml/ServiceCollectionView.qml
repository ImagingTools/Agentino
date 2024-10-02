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

    property alias clientId: serviceCollectionView.clientId
    property alias clientName: serviceCollectionView.clientName

    orientation: Qt.Vertical


    ServiceCollectionViewBase {
        id: serviceCollectionView;

        width: parent.width
        height: 200 //container.height - log.height

        onServiceIdChanged: {
            console.log("onServiceIdChanged", serviceId)
            log.serviceId = serviceId
        }

        // onSelectionChanged: {
        //     if (selection.length > 0){
        //         let index = selection[0];
        //         log.serviceId = serviceCollectionView.table.elements.getData("Id", index);
        //     }
        //     else{
        //         log.serviceId = ""
        //     }
        // }
    }

    MessageCollectionView {
        id: log

        property string serviceId

        collectionId: "ServiceLog";

        collectionFilter: MessageCollectionFilter {

        }

        onServiceIdChanged: {
            dataController.elementsModel.clear()
            if (dataController.collectionId !== log.collectionId){
                dataController.collectionId = log.collectionId
            }
            else{
                dataController.updateModel()
            }
        }

        function getAdditionalInputParams(){
            console.log("getAdditionalInputParams", container.clientId)
            let additionInputParams = {}
            additionInputParams["clientId"] = container.clientId;
            additionInputParams["serviceId"] = log.serviceId;
            return additionInputParams
        }

        function handleSubscription(dataModel){
            if (!dataModel){
                return;
            }
            if (dataModel.containsKey("OnServiceLogChanged")){
                let body = dataModel.getData("OnServiceLogChanged");
                if (body.containsKey("serviceId")){
                    let id = body.getData("serviceId")
                    if (id  === container.serviceId){
                        dataController.elementsModel.clear()
                        dataController.updateModel()
                    }
                }
            }
        }
    }
}

