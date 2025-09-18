import QtQuick 2.12
import Acf 1.0
import com.imtcore.imtqml 1.0
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
		height: 200
		
		onServiceIdChanged: {
			if (serviceId == ""){
				if (log.dataController){
					log.dataController.clearElements()
				}
			}
			else{
				log.serviceId = serviceId
				log.collectionId = "ServiceLog"
				log.doUpdateGui()
			}
		}
	}
	
	MessageCollectionView {
		id: log
		
		width: parent.width
		height: 500;
		
		visible: serviceId !== ""
		
		property string serviceId
		
		gqlGetListCommandId: "GetServiceLog"
		
		function getHeaders(){
			let additionInputParams = {}
			additionInputParams["clientid"] = container.clientId;
			additionInputParams["serviceid"] = log.serviceId;
			return additionInputParams
		}
		
		function handleSubscription(dataModel){
			if (!dataModel){
				return;
			}
			if (dataModel.containsKey("OnServiceLogChanged")){
				let body = dataModel.getData("OnServiceLogChanged");
				if (body.containsKey("serviceid")){
					let id = body.getData("serviceid")
					if (id  === log.serviceId){
						log.doUpdateGui()
					}
				}
			}
		}
	}
}

