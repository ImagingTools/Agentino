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


    RemoteCollectionView {
        id: log;

        width: parent.width
        height: 200;

        commandsControllerComp: null

        collectionId: "ServiceLog";
        property string serviceId
        property alias clientId: container.clientId

        filterMenuVisible: false;

        onHeadersChanged: {
            if (log.table.headers.GetItemsCount() > 0){
                log.table.tableDecorator = logTableDecoratorModel
            }
        }

        onServiceIdChanged: {
            dataController.elementsModel.Clear()
            dataController.updateModel()
        }

        Component.onCompleted: {
            console.log("DEBUG:log Component.onCompleted", collectionId, container.clientId)
            collectionFilter.setSortingOrder("DESC");
            collectionFilter.setSortingInfoId("Timestamp");
        }

        onClientIdChanged: {
            dataController.collectionId = log.collectionId
        }

        dataControllerComp: Component { CollectionRepresentation {
            id: messageCollectionRepresentation

            function getAdditionalInputParams(){
                console.log("LogCollectionView", container.clientId)
                return log.getAdditionalInputParams()
            }
        } }

        function getAdditionalInputParams(){
            console.log("getAdditionalInputParams", log.clientId)
            let additionInputParams = {}
            additionInputParams["clientId"] = log.clientId;
            additionInputParams["serviceId"] = log.serviceId;
            return additionInputParams
        }

        TreeItemModel {
            id: logTableDecoratorModel;

            Component.onCompleted: {
                var cellWidthModel = logTableDecoratorModel.AddTreeModel("CellWidth");

                let index = cellWidthModel.InsertNewItem();
                cellWidthModel.SetData("Width", -1, index);

                index = cellWidthModel.InsertNewItem();
                cellWidthModel.SetData("Width", 200, index);

                index = cellWidthModel.InsertNewItem();
                cellWidthModel.SetData("Width", 300, index);
            }
        }

    }

}

