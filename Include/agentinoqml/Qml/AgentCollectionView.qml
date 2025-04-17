import QtQuick 2.12
import Acf 1.0
import com.imtcore.imtqml 1.0
import imtcontrols 1.0
import imtcolgui 1.0
import imtguigql 1.0
import imtdocgui 1.0
import imtgui 1.0
import imtbaseComplexCollectionFilterSdl 1.0

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
				log.collectionId = "AgentLog"

				log.doUpdateGui()
				
				selectedServices = agentCollectionView.table.elements.getData("services", index)
			}
			else{
				log.agentId = ""
				log.dataController.clearElements()
			}
		}
	}
	
	MessageCollectionView {
		id: log
		
		property string agentId

		Component.onCompleted: {
			filterMenu.decorator = messageCollectionFilterComp;
		}
		
		function getHeaders(){
			let additionInputParams = {}
			additionInputParams["clientid"] = log.agentId;
			
			return additionInputParams
		}
		
		function handleSubscription(dataModel){
			log.doUpdateGui()
		}
		
		Component {
			id: messageCollectionFilterComp;
			
			MessageCollectionFilterDecorator {
				id: filterDecorator
				complexFilter: log.collectionFilter
				property string services: agentCollectionView.selectedServices
				
				onServicesChanged: {
					if (services != ""){
						checkMenu.dataModel.clear()
						var servicesModel = services.split(';')
						for (let i = 0; i < servicesModel.length; i++){
							checkMenu.dataModel.insertNewItem()
							checkMenu.dataModel.setData("name", servicesModel[i], i)
						}
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
					nameId: "name";

					onChangedSignal: {
						sourceGroupFilter.m_fieldFilters.clear()
						
						log.collectionFilter.removeGroupFilter(sourceGroupFilter)
						
						for (let i = 0; i < checkMenu.dataModel.getItemsCount(); i++){
							let service = checkMenu.dataModel.getData("name", i)
							let status = checkMenu.dataModel.getData("checkState", i)
							if (status){
								let filter = sourceFilter.copyMe();
								filter.m_filterValue = service
								sourceGroupFilter.m_fieldFilters.addElement(filter)
							}
						}
						
						log.collectionFilter.addGroupFilter(sourceGroupFilter)
						
						log.collectionFilter.filterChanged()
					}
				}
				
				FieldFilter {
					id: sourceFilter
					m_fieldId: "Category"
					m_filterValueType: "String"
					m_filterOperations: ["Equal"]
				}
				
				GroupFilter {
					id: sourceGroupFilter
					m_logicalOperation: "Or"
					
				}
			}
		}
	}
}
