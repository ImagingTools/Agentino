import QtQuick 2.12
import Acf 1.0
import com.imtcore.imtqml 1.0
import imtcontrols 1.0
import imtguigql 1.0

QtObject {
    id: root;

    property TreeItemModel serviceLogModel: null;

    function updateServiceLog(serviceId){
        serviceLogGqlModel.updateModel(serviceId);
    }

    function getHeaders(){
        return {};
    }

    property GqlModel serviceLogGqlModel : GqlModel {
        function updateModel(serviceId) {
            var query = Gql.GqlRequest("query", "GetServiceLog");

            var inputParams = Gql.GqlObject("input");
            inputParams.InsertField("id", serviceId);

            let additionInputParams = root.getHeaders();
            if (Object.keys(additionInputParams).length > 0){
                let additionParams = Gql.GqlObject("addition");
                for (let key in additionInputParams){
                    additionParams.InsertField(key, additionInputParams[key]);
                }
                inputParams.InsertFieldObject(additionParams);
            }
            query.AddParam(inputParams);

            var gqlData = query.GetQuery();

            this.setGqlQuery(gqlData);
        }

        onStateChanged: {
            console.log("State:", this.state, root.serviceLogGqlModel);

            if (this.state === "Ready") {
                var dataModelLocal;

                if (root.serviceLogGqlModel.containsKey("errors")){
                    dataModelLocal = root.applicationInfoQuery.getData("errors");

                    return;
                }

                if (root.serviceLogGqlModel.containsKey("data")){
                    dataModelLocal = root.serviceLogGqlModel.getData("data");

                    if (dataModelLocal.containsKey("GetServiceLog")){
                        dataModelLocal = dataModelLocal.getData("GetServiceLog");

                        root.serviceLogModel = dataModelLocal;
                    }
                }
            }
        }
    }//GetSettings
}

