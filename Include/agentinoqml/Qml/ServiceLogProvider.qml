import QtQuick 2.12
import Acf 1.0
import imtcontrols 1.0
import imtguigql 1.0

QtObject {
    id: root;

    property TreeItemModel serviceLogModel: null;

    function updateServiceLog(serviceId){
        serviceLogGqlModel.updateModel(serviceId);
    }

    function getAdditionalInputParams(){
        return {};
    }

    property GqlModel serviceLogGqlModel : GqlModel {
        function updateModel(serviceId) {
            var query = Gql.GqlRequest("query", "GetServiceLog");

            var inputParams = Gql.GqlObject("input");
            inputParams.InsertField("Id", serviceId);

            let additionInputParams = root.getAdditionalInputParams();
            if (Object.keys(additionInputParams).length > 0){
                let additionParams = Gql.GqlObject("addition");
                for (let key in additionInputParams){
                    additionParams.InsertField(key, additionInputParams[key]);
                }
                inputParams.InsertFieldObject(additionParams);
            }
            query.AddParam(inputParams);

            var gqlData = query.GetQuery();

            this.SetGqlQuery(gqlData);
        }

        onStateChanged: {
            console.log("State:", this.state, root.serviceLogGqlModel);

            if (this.state === "Ready") {
                var dataModelLocal;

                if (root.serviceLogGqlModel.ContainsKey("errors")){
                    dataModelLocal = root.applicationInfoQuery.GetData("errors");

                    return;
                }

                if (root.serviceLogGqlModel.ContainsKey("data")){
                    dataModelLocal = root.serviceLogGqlModel.GetData("data");

                    if (dataModelLocal.ContainsKey("GetServiceLog")){
                        dataModelLocal = dataModelLocal.GetData("GetServiceLog");

                        root.serviceLogModel = dataModelLocal;
                    }
                }
            }
        }
    }//GetSettings
}

