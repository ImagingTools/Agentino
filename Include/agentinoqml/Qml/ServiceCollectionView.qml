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
        table.enableAlternating: false

        collectionId: "ServiceLog";

        property string serviceId
        property alias clientId: container.clientId

        collectionFilter: MessageCollectionFilter {

        }

        TreeItemModel {
            id: collectionHeadersModel;

            Component.onCompleted: {
                log.updateHeaders();
            }
        }

        function updateHeaders(){
            collectionHeadersModel.Clear();

            let index = collectionHeadersModel.InsertNewItem();
            collectionHeadersModel.SetData("Id", "Text", index);
            collectionHeadersModel.SetData("Name", qsTr("Description"), index);

            index = collectionHeadersModel.InsertNewItem();
            collectionHeadersModel.SetData("Id", "LastModified", index);
            collectionHeadersModel.SetData("Name", qsTr("Time"), index);

            index = collectionHeadersModel.InsertNewItem();
            collectionHeadersModel.SetData("Id", "Source", index);
            collectionHeadersModel.SetData("Name", qsTr("Source"), index);

            log.dataController.headersModel  = collectionHeadersModel;
        }

        onHeadersChanged: {
            if (log.table.headers.GetItemsCount() > 0){
                log.table.tableDecorator = logTableDecoratorModel
                log.table.setColumnContentComponent(0, stateColumnContentComp);
            }
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

        Component.onCompleted: {
            console.log("DEBUG:log Component.onCompleted", collectionId, container.clientId)
            collectionFilter.setSortingOrder("DESC");
            collectionFilter.setSortingInfoId("LastModified");
            filterMenu.decorator = messageCollectionFilterComp;
        }

        function onFilterChanged(filterId, filterValue) {
            if (filterId === "TextFilter"){
                collectionFilter.setTextFilter(filterValue);
            }
            else{
                collectionFilter.setMessageStatusFilter(filterId, filterValue);
            }

            log.doUpdateGui();
        }

        dataControllerComp: Component { CollectionRepresentation {
            id: messageCollectionRepresentation
            additionalFieldIds: ["Id", "Name", "Category"]

            function getAdditionalInputParams(){
                console.log("LogCollectionView", container.clientId)
                return log.getAdditionalInputParams()
            }

            function updateModel(){
                console.log("Collection representation updateModel");
                log.doUpdateGui()
            }
        } }

        function getAdditionalInputParams(){
            console.log("getAdditionalInputParams", log.clientId)
            let additionInputParams = {}
            additionInputParams["clientId"] = log.clientId;
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
                    if (id  === serviceId){
                        dataController.elementsModel.Clear()
                        dataController.updateModel()
                    }
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

        Component {
            id: messageCollectionFilterComp;

            MessageCollectionFilterDecorator {}
        }

        Component {
            id: stateColumnContentComp;
            TableCellIconTextDelegate {
                onRowIndexChanged: {
                    if (rowIndex >= 0){
                        let category = rowDelegate.tableItem.elements.GetData("Category", rowIndex);
                        if (category === 1){
                            icon.source = "../../../../" + Style.getIconPath("Icons/Info", Icon.State.On, Icon.Mode.Normal);
                        }
                        else if (category === 2){
                            icon.source = "../../../../" + Style.getIconPath("Icons/Warning", Icon.State.On, Icon.Mode.Normal);
                        }
                        else if (category === 3){
                            icon.source = "../../../../" + Style.getIconPath("Icons/Error", Icon.State.On, Icon.Mode.Normal);
                        }
                        else if (category === 4){
                            icon.source = "../../../../" + Style.getIconPath("Icons/Critical", Icon.State.On, Icon.Mode.Normal);
                        }
                    }
                }
            }
        }
    }

}

