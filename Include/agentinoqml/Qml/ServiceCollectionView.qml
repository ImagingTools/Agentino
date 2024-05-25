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

        onSelectionChanged: {
            if (selection.length > 0){
                let index = selection[0];
                log.serviceId = serviceCollectionView.table.elements.GetData("Id", index);
            }
            else{
                log.serviceId = ""
            }
        }
    }

    MessageCollectionView {
        id: log

        property string serviceId

        collectionId: "ServiceLog";

        collectionFilter: MessageCollectionFilter {

        }

        onServiceIdChanged: {
            dataController.elementsModel.Clear()
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
            if (dataModel.ContainsKey("OnServiceLogChanged")){
                let body = dataModel.GetData("OnServiceLogChanged");
                if (body.ContainsKey("serviceId")){
                    let id = body.GetData("serviceId")
                    if (id  === container.serviceId){
                        dataController.elementsModel.Clear()
                        dataController.updateModel()
                    }
                }
            }
        }
    }
}

