import QtQuick 2.12
import Acf 1.0
import imtcontrols 1.0
import imtcolgui 1.0
import imtguigql 1.0
import imtdocgui 1.0
import imtgui 1.0

Item {
    id: container

    anchors.fill: parent

    property alias clientId: serviceCollectionView.clientId
    property alias clientName: serviceCollectionView.clientName

    ServiceCollectionViewBase {
        id: serviceCollectionView;

        anchors.top: parent.top
        anchors.left: parent.left;
        anchors.right: parent.right;
        anchors.bottom: log.top;

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

    RemoteCollectionView {
        id: log;

        anchors.left: parent.left;
        anchors.right: parent.right;
        anchors.bottom: parent.bottom;
        height: 200;

        collectionId: "ServiceLog";
        property string serviceId

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
            collectionFilter.setSortingInfoId("LastModified");

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
            let additionInputParams = {}
            additionInputParams["clientId"] = container.clientId;
            additionInputParams["serviceId"] = log.serviceId;
            return additionInputParams
        }

    }

}

