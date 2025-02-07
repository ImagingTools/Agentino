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
    }

    MessageCollectionView {
        id: log

        property string serviceId

        collectionId: "";

        collectionFilter: MessageCollectionFilter {

        }

        onServiceIdChanged: {
            dataController.elementsModel.clear()
            if (collectionId == ""){
                collectionId = "ServiceLog"
                dataController.collectionId = log.collectionId

                return;
            }

            dataController.updateModel()
			// unRegisterSubscription()
			// registerSubscription()
        }

        function getHeaders(){
            console.log("getHeaders", container.clientId)
            let additionInputParams = {}
            additionInputParams["clientid"] = container.clientId;
            additionInputParams["serviceid"] = log.serviceId;
            return additionInputParams
        }

        function handleSubscription(dataModel){
            console.log("ServiceCollectionView handleSubscription")
            if (!dataModel){
                return;
            }
            if (dataModel.containsKey("OnServiceLogChanged")){
                let body = dataModel.getData("OnServiceLogChanged");
                if (body.containsKey("serviceid")){
                    let id = body.getData("serviceid")
                    if (id  === log.serviceId){
                        dataController.elementsModel.clear()
                        dataController.updateModel()
                    }
                }
            }
        }
    }
}

