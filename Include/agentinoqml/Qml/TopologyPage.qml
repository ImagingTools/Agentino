import QtQuick 2.12
import Acf 1.0
//import imtdocgui 1.0
//import imtcolgui 1.0
import imtcontrols 1.0
//import imtguigql 1.0
import imtgui 1.0
import imtguigql 1.0
import imtcolgui 1.0


ViewBase {
    id: topologyPage;

    anchors.fill: parent;
    clip: true;

    property TreeItemModel objectModel: TreeItemModel{};

    //for scrollBars
    property real contentWidth: mainContainer.width.toFixed(3);
    property real contentHeight: mainContainer.height.toFixed(3);
    property real contentX: -mainContainer.x.toFixed(3);
    property real contentY: -mainContainer.y.toFixed(3);
    property real originX: 0;
    property real originY: 0;
    //for scrollBars

    commandsController: CommandsRepresentationProvider {
        id: commandsRepresentationProvider
        commandId: "Topology"
        uuid: topologyPage.viewId;
    }

    onContentXChanged: {
        if(mainContainer.x !== - contentX){
            mainContainer.x = - contentX;
        }
    }
    onContentYChanged: {
        if(mainContainer.y !== - contentY){
            mainContainer.y = - contentY;
        }
    }

    SchemeView {
        id: scheme

        Component.onCompleted: {
            //TEST
            //links for test
            let indexLisalink = linkLisaModel.InsertNewItem();
            linkLisaModel.SetData("ObjectId", "Puma", indexLisalink);

            let indexRTVisionlink = linkRTVisionModel.InsertNewItem();
            linkRTVisionModel.SetData("ObjectId", "Puma", indexRTVisionlink);
            indexRTVisionlink = linkRTVisionModel.InsertNewItem();
            linkRTVisionModel.SetData("ObjectId", "Lisa", indexRTVisionlink);

            let indexRTVision3dlink = linkRTVision3dModel.InsertNewItem();
            linkRTVision3dModel.SetData("ObjectId", "Puma", indexRTVision3dlink);
            indexRTVision3dlink = linkRTVisionModel.InsertNewItem();
            linkRTVision3dModel.SetData("ObjectId", "Lisa", indexRTVision3dlink);

            let index = objectModel.InsertNewItem();
            objectModel.SetData("Id", "Puma", index);
            objectModel.SetData("X", 300, index);
            objectModel.SetData("Y", 300, index);
            objectModel.SetData("MainText", "Puma service", index);
            objectModel.SetData("SecondText", "Primary authorization server", index);

            index = objectModel.InsertNewItem();
            objectModel.SetData("Id", "RTVision.3d", index);
            objectModel.SetData("X", 600, index);
            objectModel.SetData("Y", 600, index);
            objectModel.SetData("MainText", "RTVision.3d server", index);
            objectModel.SetData("SecondText", "", index);
            objectModel.SetData("HasError", true, index);
            objectModel.SetExternTreeModel("Links", linkRTVision3dModel, index);


            index = objectModel.InsertNewItem();
            objectModel.SetData("Id", "Lisa", index);
            objectModel.SetData("X", 100, index);
            objectModel.SetData("Y", 100, index);
            objectModel.SetData("MainText", "Lisa service", index);
            objectModel.SetData("SecondText", "License/features management system", index);
            objectModel.SetData("IsComposite", true, index);
            objectModel.SetExternTreeModel("Links", linkLisaModel, index);

            index = objectModel.InsertNewItem();
            objectModel.SetData("Id", "RTVision", index);
            objectModel.SetData("X", 600, index);
            objectModel.SetData("Y", 100, index);
            objectModel.SetData("MainText", "RTVision server", index);
            objectModel.SetData("SecondText", "", index);
            objectModel.SetExternTreeModel("Links", linkRTVisionModel, index);

            console.log("Start draw")
//            canvas.requestPaint();
            scheme.requestPaint();
            //TEST
        }
        TreeItemModel {id: linkLisaModel;/*for test*/}
        TreeItemModel {id: linkRTVisionModel;/*for test*/}
        TreeItemModel {id: linkRTVision3dModel;/*for test*/}
    }

}
