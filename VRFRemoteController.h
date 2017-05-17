#pragma once
#include <vrfExtProtocol/objectType.h>
#include "vrfcontrol/vrfBackendListener.h"
#include "vrfutil/compressedFileTransporter.h"
#include "vrfmsgs/ifSaveComplete.h"
#include "vrfmsgs/ifExportComplete.h"
#include "vrftasks/setDataRequest.h"
#include "vrfutil/scenario.h"
#include "vrfutil/rwUUID.h"
#include "vrftasks/simTask.h"
#include "vrfplan/plan.h"
#include <vrfExtProtocol/objectTypeEnums.h>
#include "vrfmsgs/spatialSubdivisionTypes.h"
#include "vrfmsgs/defaultVrfObjectMessageFactory.h"
#include "vrftasks/defaultSimTaskFactory.h"
#include "vrftasks/defaultSimReportFactory.h"
#include "vrftasks/defaultSetDataRequestFactory.h"
#include "vrftasks/defaultAdminContentFactory.h"
#include "vrfutil/defaultCondExprFactory.h"
#include "vrfplan/defaultStatementFactory.h"
#include "vrfplan/defaultSimCommandFactory.h"
#include "vrfcontrol/vrfcontrolDefines.h"
#include "vrfmsgs/vrfObjectMessageExecutive.h"
#include "vrfmsgs/ifModifyIndirectArtilleryObject.h"
#include "vrfMsgTransport/defaultSimulationModelSetValidatorManager.h"

#include <vl/commentInteraction.h>
#include <vl/exerciseConn.h>
#include <vl/hostStructs.h>
#include <vlpi/disEnums.h>
#include <vlpi/appearance.h>
#include <vlutil/vlHashlist.h>

#include <list>

class DtAdminContent;
class DtBackendMap;
class DtAddressMap;
class DtVrfMessageInterface;
class DtVrfConfigurationFileComposer;
class DtIfServerRequestResponse;
class DtIfLoadScenario;
class DtIfPlan;
class DtIfJoystickControlsResponse;
class DtRadioMessageListener;
class DtUUIDNetworkManager;
class DtIfMultiplePlanResponse;
class DtVrfIndirectArtilleryObjectModify;
class DtSettingsObject;
class DtReflectedEnvironmentProcessList;
class DtRemoteEnvironmentController;
class DtReflectedEntityList;
class DtReflectedAggregateList;

typedef void(*DtScenarioCallbackFcn)(void* usr);
class DtVrfOverlayObjectModify;

typedef bool(*DtBackendRemappingFcn)(DtBackendMap* map, void* usr);
typedef bool(*DtPreserveMappingsFcn)(void* usr);

typedef void(*DtJoystickControlResponseFcn)(DtIfJoystickControlsResponse* map, void* usr);

typedef void(*DtBackendSaveCallbackFcn)
(const DtSimulationAddress& addr, DtSaveResult result, void * usr);

typedef void(*DtScenarioSavedCallbackFcn)
(DtSaveResult result, const DtString& sError, void* usr);

typedef void(*DtCheckpointSavedCallbackFcn)
(DtSaveResult result, double, void* usr);

typedef void(*DtVrfObjectCreatedCallbackFcn)
(const DtString& name, const DtEntityIdentifier& id, const DtUUID& uuid, void* usr);

typedef void(*DtOverlayCreatedCallbackFcn)
(const DtString& name, const DtEntityIdentifier& id, const DtUUID& uuid, DtVrfOverlayObjectModify* usr);

typedef void(*DtIndirectArtilleryCreatedCallbackFcn)
(const DtString& name, const DtEntityIdentifier& id,
	const DtUUID& uuid, void* usr);

typedef void(*DtObjectConsoleMessageCallbackFcn)
(const DtEntityIdentifier& id, int notifyLevel, const DtString& message,
	void* usr);

typedef void(*DtBackendConsoleMessageCallbackFcn)
(const DtSimulationAddress& id, int notifyLevel, const DtString& message,
	void* usr);

typedef void(*DtServerRequestResponseCallbackFcn)
(const DtIfServerRequestResponse& response, void* usr);

class PlanCbInfo
{
public:
	PlanCbInfo(const DtUUID& n, bool global, DtPlanCallbackFcn f, void* usr) :
		planName(n), isGlobal(global), fcn(f), data(usr) {}
	~PlanCbInfo() {}
	bool operator==(const PlanCbInfo& orig) const
	{
		return (planName == orig.planName && fcn == orig.fcn && data == orig.data
			&& isGlobal == orig.isGlobal);
	}
	DtUUID planName;
	bool isGlobal;
	DtPlanCallbackFcn fcn;
	void* data;
};

class StmtCbInfo
{
public:
	StmtCbInfo(const DtUUID& n, bool global,
		DtPlanStatementCallbackFcn f, void* usr) :
		planName(n), isGlobal(global), fcn(f), data(usr) {}
	~StmtCbInfo() {}
	bool operator==(const StmtCbInfo& orig) const
	{
		return (planName == orig.planName && fcn == orig.fcn && data == orig.data &&
			isGlobal == orig.isGlobal);
	}
	DtPlanStatementCallbackFcn fcn;
	DtUUID planName;
	bool isGlobal;
	void* data;
};

class PlanCompleteCbInfo
{
public:
	PlanCompleteCbInfo(const DtUUID& n, bool global,
		DtPlanCompleteCallbackFcn f, void* usr) :
		planName(n), isGlobal(global), fcn(f), data(usr) {}
	~PlanCompleteCbInfo() {}
	bool operator==(const PlanCompleteCbInfo& orig) const
	{
		return (planName == orig.planName && fcn == orig.fcn && data == orig.data && isGlobal == orig.isGlobal);
	}
	DtPlanCompleteCallbackFcn fcn;
	DtUUID planName;
	bool isGlobal;
	void* data;
};

class DT_DLL_vrfcontrol DtVrfRemoteController
{
public:
	enum ComponentAttachmentRule {
		AttachNone = 0,
		AttachFirst,
		AttachEven

	};
public:
	DtVrfRemoteController(int receivePort = 9000);
	virtual void init(DtExerciseConn* exConn,
		DtReflectedEnvironmentProcessList* rel = NULL,
		DtReflectedEntityList* reel = NULL,
		DtReflectedAggregateList* ral = NULL,
		const DtString& uuidMarkingForNonVrForces = "entity-identifier");
	virtual void init(DtVrfMessageInterface* msgIf,
		DtReflectedEnvironmentProcessList* rel = NULL,
		DtReflectedEntityList* reel = NULL,
		DtReflectedAggregateList* ral = NULL,
		const DtString& uuidMarkingForNonVrForces = "entity-identifier");
	virtual void reinitialize(
		DtReflectedEnvironmentProcessList* rel = NULL,
		DtReflectedEntityList* reel = NULL,
		DtReflectedAggregateList* ral = NULL);
	virtual ~DtVrfRemoteController();
	virtual DtVrfMessageInterface* vrfMessageInterface() const;
	virtual DtUUIDNetworkManager* uuidNetworkManager() const;
	virtual const DtSimulationAddress& simulationAddress() const;
	virtual int sessionId() const;
	virtual double simTime(const DtSimulationAddress* a = NULL) const;
	virtual const DtList& backends() const;
	virtual const DtList& pendingBackends() const;
	virtual DtVrfBackendListener* backendListener() const;
	virtual void addBackendDiscoveryCallback(
		DtBackendCallbackFcn fcn, void* usr);
	virtual void removeBackendDiscoveryCallback(
		DtBackendCallbackFcn fcn, void* usr);
	virtual void addBackendRemovalCallback(
		DtBackendCallbackFcn fcn, void* usr);
	virtual void removeBackendRemovalCallback(
		DtBackendCallbackFcn fcn, void* usr);
	virtual int backendsControlState() const;
	virtual int numberBackendsLoaded() const;
	virtual bool allBackendsReady() const;
	virtual bool backendLoaded(const DtSimulationAddress& addr) const;
	virtual void modifyBackEndSetting(const DtSettingsObject* setting,
		const DtSimulationAddress& addr = DtSimSendToAll);
	virtual void requestBackEndSetting(std::string name,
		DtBackEndSettingRequestCompleteCallbackFcn fcn, void* usr = 0,
		const DtSimulationAddress& addr = DtSimSendToAll);
	virtual void setSpotReportsGloballyEnabled(bool enabled);
	virtual void setEngagementReportsGloballyEnabled(bool enabled);
	virtual void setPeriodicSnapshotSettings(bool enabled, double period, int numberOfSnapshotsToKeep);
	virtual void setDRThresholds(double translation, double rotation);
	virtual void setCommModelSettings(bool useExternalComms);
	virtual void setRoadDrivingSettings(bool driveOnLeft);
	virtual void setAutomaticAggregationDisaggregationEnabled(bool enabled);
	virtual void setPublishAggregateFormationRoutes(bool yesNo);
	//-------------------------------------------------------
	//-------------------------------------------------------
	virtual bool loadScenario(const DtFilename& filename,
		DtBackendRemappingFcn fcn = 0, void* usrRemapping = 0,
		DtLoadBalancingFcn = 0, void* usrBalancing = 0,
		const std::vector<DtString>& simulationModelSetFiles = std::vector<DtString>(),
		ComponentAttachmentRule compAttachRule = DtVrfRemoteController::AttachNone,
		DtString* errorString = 0);
	virtual bool loadScenario(const DtScenario& scenario,
		DtBackendRemappingFcn fcn = 0, void* usrRemapping = 0,
		DtLoadBalancingFcn = 0, void* usrBalancing = 0,
		const std::vector<DtString>& simulationModelSetFiles = std::vector<DtString>(),
		DtString* errorString = 0);
	virtual bool importScenario(const DtScenario&, const DtSimulationAddress& addr, DtString* errorString = 0);
	virtual bool importScenario(const DtFilename&, const DtSimulationAddress& addr, DtString* errorString = 0);
	virtual bool saveSimulationObjectGroup(const DtFilename& smsDirectory, const DtString& groupName,
		const DtString& groupDescription, int creationFlags, const DtSimulationAddress& addr,
		const std::map<DtString, DtString>& additionalInformation = std::map<DtString, DtString>(),
		DtString* saveError = 0);
	virtual void rollbackToSnapshot(double simTime);
	virtual bool importScenarioDescriptionFile(const DtFilename& file, DtSimulationAddress, const DtString& format,
		const std::map<std::string, std::string>& importData);
	virtual bool exportScenarioDescriptionFile(const DtFilename& file, DtSimulationAddress, const DtString& format,
		const std::map<std::string, std::string>& importData);
	virtual DtString guiTerrainDatabaseFilename() const;
	virtual void setGuiTerrainDatabaseFilename(DtString db);
	virtual const DtRwKeyValueList& scenarioData() const;
	virtual void setScenarioData(const DtRwKeyValueList&);
	virtual bool newScenario(
		const DtFilename& dbname,
		const DtFilename& guidbname,
		const DtFilename& opdname = DtFilename::nullFilename(),
		const DtFilename& physicalWorldName = DtFilename::nullFilename(),
		const DtScenario* baseScenario = 0,
		time_t startTimeOfDay = 12 * 3600,
		DtString* errorString = 0);
	virtual bool newScenario(
		const DtFilename& dbname,
		const DtFilename& guidbname,
		const std::vector<DtString>& simulationModelSetFiles,
		const DtScenario* baseScenario = 0,
		time_t startTimeOfDay = 12 * 3600,
		DtString* errorString = 0);
	virtual bool sendLoadScenarioMessage(
		const DtScenario& scenario,
		const DtSimulationAddress& addr = DtSimSendToAll,
		bool import = false) const;
	virtual DtIfLoadScenario* createLoadScenarioMessage() const;
	virtual void addBackendLoadedCallback(DtBackendCallbackFcn fcn, void* usr);
	virtual void removeBackendLoadedCallback(DtBackendCallbackFcn fcn, void* usr);
	virtual void addScenarioLoadedCallback(DtScenarioCallbackFcn fcn, void* usr);
	virtual void removeScenarioLoadedCallback(DtScenarioCallbackFcn fcn, void* usr);
	virtual bool isSavingScenario() const;
	virtual bool saveScenario(const DtFilename& saveName,
		DtPreserveMappingsFcn fcn = 0, void* userData = 0, DtString* error = 0,
		double simTime = -1);
	virtual bool saveScenarioCheckpoint(const DtFilename& filename, double simTime);
	virtual void takeSnapshot(DtSimulationAddress addr = DtSimSendToAll);
	virtual void takeBaseSnapshot();
	virtual void sendSaveScenarioMessage(const DtSimulationAddress& addr = DtSimSendToAll, double simTime = -1,
		const DtFilename& saveScenarioFilename = "") const;
	virtual void addBackendSavedCallback(DtBackendSaveCallbackFcn fcn, void* usr);
	virtual void removeBackendSavedCallback(DtBackendSaveCallbackFcn fcn, void* usr);
	virtual void addScenarioSavedCallback(
		DtScenarioCallbackFcn fcn, void* usr);
	virtual void removeScenarioSavedCallback(
		DtScenarioCallbackFcn fcn, void* usr);
	virtual void addScenarioExportedCallback(
		DtScenarioSavedCallbackFcn fcn, void* usr);
	virtual void removeScenarioExportedCallback(
		DtScenarioSavedCallbackFcn fcn, void* usr);
	virtual void addCheckpointSavedCallback(
		DtCheckpointSavedCallbackFcn fcn, void* usr);
	virtual void removeCheckpointSavedCallback(
		DtCheckpointSavedCallbackFcn fcn, void* usr);
	virtual void addScenarioSavedCallback(
		DtScenarioSavedCallbackFcn fcn, void* usr);
	virtual void removeScenarioSavedCallback(
		DtScenarioSavedCallbackFcn fcn, void* usr);
	virtual void closeScenario(const DtSimulationAddress& addr = DtSimSendToAll,
		const DtFilename& databaseToOpen = "", bool forceCloseAll = false);
	virtual void run(bool useStartStopInteraction = false, const DtSimulationAddress& addr = DtSimSendToAll) const;
	virtual void pause(bool useStartStopInteraction = false, const DtSimulationAddress& addr = DtSimSendToAll) const;
	virtual void rewind() const;
	virtual void exit() const;
	virtual void step() const;
	virtual void setTimeMultiplier(double multiplier) const;
	virtual void setOpdSaveName(const DtFilename& f);
	virtual const DtFilename& opdSaveName() const;
	virtual void setMaxWaitSaveTime(DtTime t);
	virtual DtTime maxWaitSaveTime() const;
	virtual void setMaxWaitLoadTime(DtTime t);
	virtual DtTime maxWaitLoadTime() const;
	virtual void setOverlayFilename(const DtFilename& overlay);
	virtual const DtFilename& overlayFilename() const;
	virtual void setHostilityFilename(const DtFilename& hostilty);
	virtual const DtFilename& hostilityFilename() const;
	virtual void setScenarioExtrasFilename(const DtFilename& extras);
	virtual const DtFilename& scenarioExtrasFilename() const;
	virtual void setSelectionGroupsFilename(const DtFilename& sel);
	virtual const DtFilename& selectionGroupsFilename() const;
	virtual void setSymbolMapFilename(const DtFilename& overlay);
	virtual const DtFilename& symbolMapFilename() const;
	virtual void setCreateMenuFilename(const DtFilename& overlay);
	virtual const DtFilename& createMenuFilename() const;
	virtual void saveParameterDb(const DtFilename& filename);
	//-------------------------------------------------------
	//-------------------------------------------------------
	virtual void deleteObject(const DtUUID& uuid,
		const DtSimulationAddress& addr = DtSimSendToAll) const;
	//-------------------------------------------------------
	//-------------------------------------------------------
	virtual void createWaypoint(
		const DtVector& geocentricPosition,
		const DtString& uniqueName = DtString::nullString(),
		const DtString& label = DtString::nullString(),
		const DtSimulationAddress& addr = DtSimSendToAll) const;
	virtual void createWaypoint(
		DtVrfObjectCreatedCallbackFcn fcn, void* usr,
		const DtVector& geocentricPosition,
		const DtString& uniqueName = DtString::nullString(),
		const DtString& label = DtString::nullString(),
		const DtSimulationAddress& addr = DtSimSendToAll);
	virtual void modifyWaypoint(
		const DtUUID& uuid,
		DtString* uniqueName = 0,
		DtString* label = 0,
		DtVector* geocentricPosition = 0,
		const DtSimulationAddress& addr = DtSimSendToAll) const;
	virtual void createRoute(
		const DtList& vertices,
		const DtString& uniqueName = DtString::nullString(),
		const DtString& label = DtString::nullString(),
		const DtSimulationAddress& addr = DtSimSendToAll) const;
	virtual void createRoute(
		DtVrfObjectCreatedCallbackFcn fcn, void* usr,
		const DtList& vertices,
		const DtString& uniqueName = DtString::nullString(),
		const DtString& label = DtString::nullString(),
		const DtSimulationAddress& addr = DtSimSendToAll);
	virtual void modifyRoute(
		const DtUUID& uuid,
		DtString* uniqueName = 0,
		DtString* label = 0,
		DtList* vertices = 0,
		const DtSimulationAddress& addr = DtSimSendToAll) const;
	virtual void createPhaseLine(
		const DtVector& dbPoint1,
		const DtVector& dbPoint2,
		const DtString& uniqueName = DtString::nullString(),
		const DtString& label = DtString::nullString(),
		const DtSimulationAddress& addr = DtSimSendToAll) const;
	virtual void createPhaseLine(
		DtVrfObjectCreatedCallbackFcn fcn, void* usr,
		const DtVector& dbPoint1,
		const DtVector& dbPoint2,
		const DtString& uniqueName = DtString::nullString(),
		const DtString& label = DtString::nullString(),
		const DtSimulationAddress& addr = DtSimSendToAll);
	virtual void modifyPhaseLine(
		const DtUUID& uuid,
		DtString* uniqueName = 0,
		DtString* label = 0,
		DtVector* newDbPoint1 = 0,
		DtVector* newDbPoint2 = 0,
		const DtSimulationAddress& addr = DtSimSendToAll) const;
	virtual void createControlArea(
		const DtList& vertices,
		const DtString& uniqueName = DtString::nullString(),
		const DtString& label = DtString::nullString(),
		const DtSimulationAddress& addr = DtSimSendToAll) const;
	virtual void createControlArea(
		DtVrfObjectCreatedCallbackFcn fcn, void* usr,
		const DtList& vertices,
		const DtString& uniqueName = DtString::nullString(),
		const DtString& label = DtString::nullString(),
		const DtSimulationAddress& addr = DtSimSendToAll);
	virtual void modifyControlArea(
		const DtUUID& uuid,
		DtString* uniqueName = 0,
		DtString* label = 0,
		DtList* newVertices = 0,
		const DtSimulationAddress& addr = DtSimSendToAll) const;
	virtual void createObstacle(
		const DtList& vertices,
		const DtString& uniqueName = DtString::nullString(),
		const DtString& label = DtString::nullString(),
		const DtSimulationAddress& addr = DtSimSendToAll) const;
	virtual void createObstacle(
		DtVrfObjectCreatedCallbackFcn fcn, void* usr,
		const DtList& vertices,
		const DtString& uniqueName = DtString::nullString(),
		const DtString& label = DtString::nullString(),
		const DtSimulationAddress& addr = DtSimSendToAll);
	virtual void modifyObstacle(
		const DtUUID& uuid,
		DtString* uniqueName = 0,
		DtString* label = 0,
		DtList* newVertices = 0,
		const DtSimulationAddress& addr = DtSimSendToAll) const;
	virtual void createDisaggregationArea(
		const DtList& vertices,
		const DtString& uniqueName = DtString::nullString(),
		const DtString& label = DtString::nullString(),
		const DtSimulationAddress& addr = DtSimSendToAll) const;
	virtual void createDisaggregationArea(
		DtVrfObjectCreatedCallbackFcn fcn, void* usr,
		const DtList& vertices,
		const DtString& uniqueName = DtString::nullString(),
		const DtString& label = DtString::nullString(),
		const DtSimulationAddress& addr = DtSimSendToAll);
	virtual void modifyDisaggregationArea(
		const DtUUID& uuid,
		DtString* uniqueName = 0,
		DtString* label = 0,
		DtList* newVertices = 0,
		const DtSimulationAddress& addr = DtSimSendToAll) const;
	virtual void createOverlayObject(
		const DtEntityType& type,
		const DtList& vertices,
		const DtForceType& force,
		const DtString& name,
		const DtString& label,
		int color,
		const DtString& parentName,
		bool projection,
		bool closedFigure,
		const DtAppearance& appearance = DtAppearance::nullAppearance(),
		const DtString& attachTo = DtString(),
		char* userData = 0,
		int dataSize = 0,
		const std::map<std::string, std::string>& extendedData = std::map<std::string, std::string>(),
		const std::map<int, DtString>& extendedLabels = std::map<int, DtString>(),
		DtOverlayCreatedCallbackFcn fcn = 0,
		const DtSimulationAddress& addr = DtSimSendToAll,
		unsigned int rId = 0,
		std::list<DtAdminContent*>* initialAdminContent = 0,
		std::list<DtSimInterfaceContent*>* initialInterfaceContent = 0);
	virtual void createIndirectArtillery(
		const DtVrfIndirectArtilleryObjectModify * params,
		const DtSimulationAddress& addr);
	//-------------------------------------------------------
	//-------------------------------------------------------
	virtual void createObject(
		const DtEntityType& type,
		DtForceType force,
		DtList* vertices,
		const DtString& name,
		const DtSimulationAddress& addr);
	virtual void createEntity(
		const DtEntityType& type,
		const DtVector& geocentricPosition,
		DtForceType force,
		DtReal heading,
		const DtString& uniqueName = DtString::nullString(),
		const DtString& label = DtString::nullString(),
		const DtSimulationAddress& addr = DtSimSendToAll,
		bool groundClamp = true) const;
	virtual void createEntity(
		DtVrfObjectCreatedCallbackFcn fcn, void* usr,
		const DtEntityType& type,
		const DtVector& geocentricPosition,
		DtForceType force,
		DtReal heading,
		const DtString& uniqueName = DtString::nullString(),
		const DtString& label = DtString::nullString(),
		const DtSimulationAddress& addr = DtSimSendToAll,
		bool groundClamp = true);
	virtual void createAggregate(
		DtVrfObjectCreatedCallbackFcn fcn, void* usr,
		const DtEntityType& type,
		const DtVector& geocentricPosition,
		DtForceType force,
		DtReal heading,
		const DtString& uniqueName,
		const DtString& label = DtString::nullString(),
		const DtSimulationAddress& addr = DtSimSendToAll,
		DtAggregateState initialAggregateState = DtDisaggregated);
	virtual void createAggregate(
		const DtEntityType& type,
		const DtVector& geocentricPosition,
		DtForceType force,
		DtReal heading,
		DtList* entityNames = 0,
		const DtSimulationAddress& addr = DtSimSendToAll,
		DtAggregateState initialAggregateState = DtDisaggregated,
		unsigned int requestId = 0) const;
	virtual void createAggregate(
		DtVrfObjectCreatedCallbackFcn fcn, void* usr,
		const DtEntityType& type,
		const DtVector& geocentricPosition,
		DtForceType force,
		DtReal heading,
		DtList* entityNames = 0,
		const DtSimulationAddress& addr = DtSimSendToAll,
		DtAggregateState initialAggregateState = DtDisaggregated);
	virtual void addToOrganization(
		const DtUUID& objectId,
		const DtUUID& newSuperiorId,
		const DtSimulationAddress& addr = DtSimSendToAll);
	virtual void addToForceLevelOrganization(
		const DtUUID& uuid,
		const DtSimulationAddress& addr = DtSimSendToAll);
	virtual void removeFromOrganization(
		const DtUUID& uuid,
		const DtSimulationAddress& addr = DtSimSendToAll);
	virtual void setLabel(
		const DtUUID& uuid, const DtString& label,
		const DtSimulationAddress& addr = DtSimSendToAll) const;
	virtual void setOverlayParent(
		const DtUUID& uuid, const DtString& label,
		const DtSimulationAddress& addr = DtSimSendToAll) const;
	virtual void setAltitude(
		const DtUUID& uuid, double altitude, bool aboveGroundLevel = false,
		const DtSimulationAddress& addr = DtSimSendToAll) const;
	virtual void setLocation(
		const DtUUID& uuid, DtVector coord,
		const DtSimulationAddress& addr = DtSimSendToAll) const;
	virtual void setAppearance(
		const DtUUID& uuid, int appearance,
		const DtSimulationAddress& addr = DtSimSendToAll) const;
	virtual void setCapabilities(
		const DtUUID& uuid, int capabilities,
		const DtSimulationAddress& addr = DtSimSendToAll) const;
	virtual void setHeading(
		const DtUUID& uuid, double degrees,
		const DtSimulationAddress& addr = DtSimSendToAll) const;
	virtual void setForce(
		const DtUUID& uuid, DtForceType force,
		const DtSimulationAddress& addr = DtSimSendToAll) const;
	virtual void requestSpotReports(const DtUUID& uuid, bool onOff,
		const DtSimulationAddress& addr = DtSimSendToAll) const;
	virtual void requestEngagementReports(const DtUUID& uuid, bool onOff,
		const DtSimulationAddress& addr = DtSimSendToAll) const;
	virtual void setResource(const DtUUID& uuid,
		const char* resourceType, double value,
		const DtSimulationAddress& addr = DtSimSendToAll) const;
	virtual void setRulesOfEngagement(
		const DtUUID& uuid, const char* rulesType,
		const DtSimulationAddress& addr = DtSimSendToAll) const;
	virtual void setSectorOfResponsibility(
		const DtUUID& uuid, double center, double size,
		const DtSimulationAddress& addr = DtSimSendToAll) const;
	virtual void setSpeed(
		const DtUUID& uuid, double speed,
		const DtSimulationAddress& addr = DtSimSendToAll) const;
	virtual void setCurrentSpeed(
		const DtUUID& uuid, double speed,
		const DtSimulationAddress& addr = DtSimSendToAll) const;
	virtual void setTarget(
		const DtUUID& uuid, const DtUUID& target,
		const DtSimulationAddress& addr = DtSimSendToAll) const;
	virtual void setSuperior(
		const DtUUID& uuid, const DtUUID& superior,
		const DtSimulationAddress& addr = DtSimSendToAll) const;
	virtual void setTaskedBySuperior(const DtUUID& uuid,
		const DtSimulationAddress& addr = DtSimSendToAll) const;
	virtual void setConcealed(const DtUUID& uuid,
		bool yes, const DtSimulationAddress& addr = DtSimSendToAll) const;
	virtual void setEmitter(const DtUUID& uuid,
		int emitterId, bool on,
		const DtSimulationAddress& addr = DtSimSendToAll) const;
	virtual void setIff(const DtUUID& uuid,
		bool mode1on, int mode1code,
		bool mode2on, int mode2code,
		bool mode3on, int mode3code,
		bool mode4on, int mode4code,
		DtIffAtcNavaidsAltParm4 altParm4,
		bool modeCon, bool useAltModeC,
		bool modeSon, bool emergency,
		bool flash, bool sti, bool modeSTcasI,
		int pin, bool mode5LevelSelection,
		unsigned int messageFormatsPresent, unsigned int nationalOrigin,
		unsigned int navigationSource, unsigned int figureOfMerit,
		const DtSimulationAddress& addr = DtSimSendToAll) const;
	virtual void restore(const DtUUID& uuid,
		const DtSimulationAddress& addr = DtSimSendToAll) const;
	virtual void setDestroyed(const DtUUID& uuid,
		const DtSimulationAddress& addr = DtSimSendToAll) const;
	virtual void setLifeformWeaponState(
		const DtUUID& uuid, DtWeaponState weapon,
		const DtSimulationAddress& addr) const;
	virtual void setLifeformPosture(
		const DtUUID& uuid, DtLifeformState posture,
		const DtSimulationAddress& addr) const;
	virtual void setAggregateFormation(
		const DtUUID& leader, const DtString& formationName,
		const DtSimulationAddress& addr = DtSimSendToAll) const;
	virtual void reorganizeAggregate(const DtUUID& leader,
		const DtSimulationAddress& addr = DtSimSendToAll) const;
	virtual void disaggregateEntity(const DtUUID& entity,
		const DtSimulationAddress& addr = DtSimSendToAll) const;
	virtual void aggregateEntity(const DtUUID& entity,
		const DtSimulationAddress& addr = DtSimSendToAll) const;
	virtual void sendSetDataMsg(
		const DtUUID& recipient, DtSetDataRequest* msg,
		const DtSimulationAddress& addr) const;
	//-------------------------------------------------------
	//-------------------------------------------------------
	virtual void sendVrfObjectCreateMsg(
		const DtSimulationAddress& addr,
		unsigned int requestId,
		const DtString& name,
		const DtObjectType& type,
		const DtList& vertices,
		const DtString& label,
		const DtAppearance& appearance,
		DtForceType force,
		bool projection,
		bool closedFigure,
		DtReal heading,
		bool createSubObjects,
		bool clampToGroundFlag = true,
		const DtString& initialFormation = DtString::nullString(),
		DtAggregateState initialAggregateState = DtDisaggregated,
		const DtString& userData = DtString::nullString(),
		const std::map<std::string, std::string>& extendedData = std::map<std::string, std::string>(),
		const std::map<int, DtString>& extendedLabels = std::map<int, DtString>(),
		std::list<DtAdminContent*>* initialAdminContent = 0,
		std::list<DtSimInterfaceContent*>* initialInterfaceContent = 0,
		const DtString& overlayParent = "") const;
	virtual void sendVrfObjectModifyMsg(
		const DtSimulationAddress& addr,
		const DtUUID& uuid,
		DtString* name = 0,
		DtList* vertices = 0,
		DtForceType* force = 0,
		DtReal* heading = 0,
		DtString* label = 0,
		DtAppearance* appearance = 0,
		DtString* overlayParent = 0,
		DtString* userData = 0) const;
	virtual void setPhysicalLineWidth(const DtSimulationAddress& addr,
		const DtUUID& entityId, double w);
	virtual void setPhysicalLineHeight(const DtSimulationAddress& addr,
		const DtUUID& entityId, double h);
	virtual void setCeiling(const DtSimulationAddress& addr,
		const DtUUID& entityId, double c);
	virtual void sendVrfOverlayObjectModifyMsg(
		const DtSimulationAddress& addr,
		const DtUUID& uuid,
		int color,
		const DtString& parentName,
		const DtUUID& attachTo,
		bool keepExistingAttachment = true,
		char* userData = 0,
		int len = 0) const;
	virtual void sendVrfIndirectArtilleryObjectModifyMsg(
		const DtSimulationAddress& addr,
		const DtUUID& uuid,
		const DtVrfIndirectArtilleryObjectModify * mod) const;
	//-------------------------------------------------------
	//-------------------------------------------------------
	virtual void moveAlongRoute(
		const DtUUID& entityName, const DtUUID& routeName,
		const DtSimulationAddress& addr = DtSimSendToAll) const;
	virtual void moveToWaypoint(
		const DtUUID& entityName, const DtUUID& waypoint,
		const DtSimulationAddress& addr = DtSimSendToAll) const;
	virtual void moveToLocation(
		const DtUUID& entityName, const DtVector& destinationLocation,
		const DtSimulationAddress& addr = DtSimSendToAll) const;
	virtual void patrolBetweenWaypoints(
		const DtUUID& uuid,
		const DtUUID& waypoint1, const DtUUID& waypoint2,
		const DtSimulationAddress& addr = DtSimSendToAll) const;
	virtual void patrolAlongRoute(
		const DtUUID& uuid, const DtUUID& route,
		const DtSimulationAddress& addr = DtSimSendToAll) const;
	virtual void followEntity(
		const DtUUID& uuid,
		const DtUUID& leader, const DtVector& offset,
		const DtSimulationAddress& addr = DtSimSendToAll) const;
	virtual void wait(
		const DtUUID& uuid,
		const DtSimulationAddress& addr = DtSimSendToAll) const;
	virtual void waitDuration(
		const DtUUID& uuid, DtTime duration,
		const DtSimulationAddress& addr = DtSimSendToAll) const;
	virtual void waitElapsed(
		const DtUUID& uuid, DtTime elapsed,
		const DtSimulationAddress& addr = DtSimSendToAll) const;
	virtual void sendTaskMsg(
		const DtUUID& recipient, DtSimTask* task,
		const DtSimulationAddress& addr = DtSimSendToAll) const;
	virtual void abandonPlan(const DtUUID& uuid,
		const DtSimulationAddress& addr = DtSimSendToAll) const;
	virtual void skipTask(const DtUUID& uuid,
		const DtSimulationAddress& addr = DtSimSendToAll) const;
	virtual void restartPlan(const DtUUID& uuid,
		const DtSimulationAddress& addr = DtSimSendToAll) const;
	virtual void assignPlanByName(
		const DtUUID& uuid, const DtPlan& plan,
		const DtSimulationAddress& addr = DtSimSendToAll,
		int *retiStatus = NULL, DtString* retsStmt = NULL) const;
	virtual void assignMultiplePlanByName(
		const std::vector<DtUUID>& entities, const DtPlan& plan,
		const DtSimulationAddress& addr = DtSimSendToAll,
		int *retiStatus = NULL, DtString* retsStmt = NULL) const;
	virtual void requestPlan(
		const DtUUID& uuid,
		bool addStmtCallback = true,
		const DtSimulationAddress& addr = DtSimSendToAll) const;
	virtual void subscribePlan(const DtUUID& uuid,
		DtPlanCallbackFcn fn, void* usr,
		const DtSimulationAddress& addr = DtSimSendToAll);
	virtual void requestMultiplePlan(const std::vector<DtUUID>& entities,
		DtMultiplePlanRequestCallbackFcn fcn, void *usr,
		const DtSimulationAddress& addr = DtSimSendToAll);
	virtual void requestMultiplePlan(const std::vector<DtUUID>& entities,
		const DtSimulationAddress& addr = DtSimSendToAll);
	virtual void unsubscribePlan(const DtUUID& uuid,
		DtPlanCallbackFcn fn, void* usr);
	virtual void unsubscribeAllPlans();
	virtual void addPlanStatementCallback(const DtUUID& uuid,
		DtPlanStatementCallbackFcn callback, void* usrData);
	virtual void removePlanStatementCallback(const DtUUID& uuid,
		DtPlanStatementCallbackFcn callback, void* usrData);
	virtual void removeAllPlanStatementCallbacks();
	virtual void addPlanCompleteCallback(const DtUUID& uuid,
		DtPlanCompleteCallbackFcn callback, void* usrData);
	virtual void removePlanCompleteCallback(const DtUUID& uuid,
		DtPlanCompleteCallbackFcn callback, void* usrData);
	virtual void removeAllPlanCompleteCallbacks();
	virtual void requestGlobalPlansList(const DtSimulationAddress& addr);
	virtual void assignGlobalPlan(
		const DtString& name, const DtPlan& plan,
		const DtSimulationAddress& addr = DtSimSendToAll,
		int *retiStatus = NULL, DtString* retsStmt = NULL) const;
	virtual void deleteGlobalPlan(const DtUUID& name,
		const DtSimulationAddress& addr = DtSimSendToAll) const;
	virtual void launchGlobalPlan(const DtUUID&, const DtSimulationAddress& addr) const;
	virtual DtVrfObjectMessageExecutive* objectMessageExecutive();
	virtual void sendMessageToObject(DtVrfObjectMessage* objectMessage,
		const DtSimulationAddress& addr = DtSimSendToAll) const;
	//-------------------------------------------------------
	virtual void loadedScenario(
		const DtSimulationAddress& addr, unsigned int requestId);
	virtual void processSaveMessages();
	virtual DtTime scenarioSimTimeForSave();
	virtual void setTerrainDatabaseFilename(DtScenario* scen);
	virtual void setParameterDatabaseFilename(DtScenario* scen);
	virtual void setPhysicalWorldFilename(DtScenario* scen);
	virtual void setTimeMultiplier(DtScenario* scen);
	virtual void setAutoReorganize(DtScenario* scen);
	virtual void setRandomNumberSeed(DtScenario* scen);
	virtual void setFrameRateMode(DtScenario* scen);
	virtual void setFrameDeltaTime(DtScenario* scen);
	virtual void setExerciseStartTime(DtScenario* scen);
	virtual void setExerciseStartDateAndTime(DtScenario* scen);
	virtual bool save(DtString& sError);
	virtual void createSimulationObjectGroup(const DtSimulationAddress&, const DtString& simulationObjectGroup,
		const DtVector& createPosition, double offsetRotation, int createFlags, DtForceType force);
	virtual void save(const DtSimulationAddress& addr, DtIfSaveComplete* s);
	virtual void exported(const DtSimulationAddress& addr, DtIfExportComplete* s);
	virtual void completeExport();
	virtual void savedScenario(const DtSimulationAddress& addr,
		DtSaveResult result, unsigned int requestId, bool isCheckpointSave = false, double checkpointSimTime = -1);
	virtual void doPendingSaveWork();
	virtual void doPendingExportWork();
	virtual void doPendingLoadWork();
	virtual void objectCreated(unsigned int requestId,
		const DtString& name, const DtEntityIdentifier& id, const DtUUID& uuid);
	virtual unsigned int generateRequestId() const;
	virtual void receivedPlan(const DtUUID& uuid, bool isGlobal,
		const std::vector<DtSimStatement*>& statements);
	virtual void receivedPlanStmt(const DtUUID& uuid, bool isGlobal,
		DtPlanStatus planStatus);
	virtual void embarkObject(const DtSimulationAddress& addr, const DtUUID& entity, const DtUUID& ontoEntity,
		bool loadOverride = false, const DtVector& loadOverridePosition = DtVector::zero());
	virtual void disembarkObject(const DtSimulationAddress& addr, const DtUUID& entity);
	virtual void disembarkAllObjects(const DtSimulationAddress& addr, const DtUUID& entity);
	virtual void requestTasksAndSetsFor(const DtSimulationAddress& addr,
		const DtUUID& entityName);
	virtual void requestTasksAndSetsFor(const std::vector<DtUUID>& names);
	virtual void requestDIGuyAppearancesAndAnimations(const DtSimulationAddress& addr,
		const DtUUID& entityName);
	virtual void requestDIGuyAppearancesAndAnimations(const std::vector<DtUUID>& names);
	//-------------------------------------------------------
	//-------------------------------------------------------
	virtual DtDefaultVrfObjectMessageFactory* vrfObjectMsgFactory();
	virtual DtDefaultCondExprFactory* condExprFactory();
	virtual DtDefaultStatementFactory* stmtFactory();
	virtual DtDefaultSimReportFactory* reportFactory();
	virtual DtDefaultSimTaskFactory* taskFactory();
	virtual DtDefaultSetDataRequestFactory* setDataFactory();
	virtual DtDefaultSimCommandFactory* simCommandFactory();
	virtual DtDefaultAdminContentFactory* adminContentFactory();
	//-------------------------------------------------------
	//-------------------------------------------------------
	virtual void setRotationThreshold(DtReal thresh);
	virtual void setTranslationThreshold(DtReal thresh);
	virtual void setAggDimensionThreshold(DtReal thresh);
	virtual void setHostInetAddr(DtInetAddr);
	virtual DtInetAddr hostInetAddr() const;
	virtual void querySpatialSubdivisionConfiguration(
		const DtSimulationAddress& backendAddress,
		DtSpatialSubdivisionType spatialSubdivisionType,
		DtServerRequestResponseCallbackFcn fcn = 0, void* usr = 0);
	virtual void setSpatialSubdivsionConfiguration(
		const DtSimulationAddress& backendAddress,
		DtSpatialSubdivisionType spatialSubdivisionType,
		const int spatialSubdivisionXResolution,
		const int spatialSubdivisionYResolution,
		const int spatialSubdivisionZResolution);
	virtual void requestIndirectArtilleryInformation(const DtUUID&);
	virtual void requestResources(const DtUUID& uuid = DtString::nullString(),
		const std::list<DtString>* list = 0,
		DtServerRequestResponseCallbackFcn fcn = 0, void* usr = 0);
	//-------------------------------------------------------
	//-------------------------------------------------------
	virtual void addObjectConsoleMessageCallback(
		DtObjectConsoleMessageCallbackFcn fcn, void* usr = 0);
	virtual void removeObjectConsoleMessageCallback(
		DtObjectConsoleMessageCallbackFcn fcn, void* usr = 0);
	virtual void setObjectNotifyLevel(const DtUUID& objectName,
		DtNotifyLevelType notifyLevel, const DtSimulationAddress& addr = DtSimSendToAll);
	virtual void logObjectConsoleToFile(const DtUUID& objectName,
		const DtFilename filename);
	virtual void stopLoggingObjectConsoleToFile(const DtUUID& objectName);
	//-------------------------------------------------------
	//-------------------------------------------------------
	virtual void addBackendConsoleMessageCallback(
		DtBackendConsoleMessageCallbackFcn fcn, void* usr = 0);
	virtual void removeBackendConsoleMessageCallback(
		DtBackendConsoleMessageCallbackFcn fcn, void* usr = 0);
	virtual void requestResourceNames(const std::vector<DtUUID>& list,
		DtServerRequestResponseCallbackFcn fcn = 0, void* usr = 0);
	virtual DtRadioMessageListener* radioMessageListener();
	//-------------------------------------------------------
	//-------------------------------------------------------
	DtRemoteEnvironmentController* remoteEnvironmentController();
	virtual void initializeGlobalEnvironment(
		const DtSimulationAddress& environmentSimBackend = DtSimSendToAll);
	DtRemoteEnvironmentController* createRemoteEnvironmentController();
	//-------------------------------------------------------
	//-------------------------------------------------------
	virtual void requestJoystickInformation(const DtUUID&, DtJoystickControlResponseFcn, void*,
		const DtSimulationAddress& addr = DtSimSendToAll);
	virtual void takeControlOf(const DtUUID&, const std::set<DtString>& functionGroups,
		const DtSimulationAddress& addr = DtSimSendToAll);
	virtual void releaseControlOf(const DtUUID&, const std::set<DtString>& functionGroups =
		std::set<DtString>(), const DtSimulationAddress& addr = DtSimSendToAll);
	virtual void sendJoystickControl(const DtUUID&, const DtString& functionGroup,
		const DtString& function, double value, bool repeat = false, const DtSimulationAddress& addr = DtSimSendToAll);
public:
	static void doPendingSaveWorkCb(void* usr);
	static void doPendingExportWorkCb(void* usr);
	static void doPendingLoadWorkCb(void* usr);
	static void savedScenarioCb(DtSimMessage* msg, void* usr);
	static void exportScenarioCb(DtSimMessage* msg, void* usr);
	static void loadedScenarioCb(DtSimMessage* msg, void* usr);
	static void receiveFileCb(const DtFilename& filename, void* usr);
	static void receivedMultiPlanCb(DtSimMessage* msg, void* usr);
	virtual void processReceivedMultiPlan(DtIfMultiplePlanResponse* msg);
	static void objectCreatedCb(DtSimMessage* msg, void* usr);
	static void receivedPlanCb(DtSimMessage* msg, void* usr);
	static void receivedPlanStmtCb(DtSimMessage* msg, void* usr);
	static void overlayObjectCreatedCallback(const DtString& name, const DtEntityIdentifier& id,
		const DtUUID& uuid, void* usr);
	virtual void processOverlayObjectCreated(const DtString& name, const DtEntityIdentifier& id, const DtUUID& uuid,
		DtVrfOverlayObjectModify* modifications);
	static void indirectArtilleryObjectCreatedCallback(const DtString& name, const DtEntityIdentifier& id,
		const DtUUID& uuid, void* usr);
	virtual void processIndirectArtilleryObjectCreated(const DtString& name, const DtEntityIdentifier& id, const DtUUID& uuid,
		DtVrfIndirectArtilleryObjectModify* modifications);
	static void resourcesProcessedCb(DtSimMessage* msg, void* usr);
	virtual void processResourcesProcessedCb(const DtIfServerRequestResponse& rsp);
	static void spatialSubdivisionConfigurationCb(DtSimMessage* msg, void* usr);
	virtual void processSpatialSubdivisionConfigurationCb(
		const DtIfServerRequestResponse& rsp);
	static void commentInteractionCallback(DtCommentInteraction *inter, void* usr);
	virtual void processCommentInteraction(DtCommentInteraction* inter);
	virtual DtVrfConfigurationFileComposer* composer();
protected:
	static void joystickControlResponse(DtSimMessage* msg, void* usr);
	virtual void processJoystickControlResponse(DtSimMessage* msg);
	virtual void onInit();
	virtual void resetStateData();
	virtual void emptyLists();
	virtual void sendAggregateAsMsg(
		const DtSimulationAddress& addr,
		unsigned int requestId,
		const DtObjectType& objectType,
		DtForceType force,
		const DtVector& position,
		DtReal heading,
		DtList* entityNames,
		DtAggregateState initialAggregateState) const;
	virtual DtVrfConfigurationFileComposer* createComposer();
	virtual DtUUIDNetworkManager* createUUIDNetworkManager() const;
	virtual void sendQuerySpatialSubdivisionConfigurationRequest(
		const DtSimulationAddress& backendAddress,
		DtSpatialSubdivisionType spatialSubdivisionType,
		DtServerRequestResponseCallbackFcn fcn, void* usr);
	virtual bool arbitrarySimAddress(DtSimulationAddress& address) const;
	virtual bool addStatementsAndSendPlan(const DtPlan& plan, DtIfPlan& planMsg,
		const DtSimulationAddress& addr = DtSimSendToAll,
		int *retiStatus = 0, DtString* retsStmt = 0) const;
	static void deliverRadioMessageCallback(DtSimMessage* msg, void* usr);
	static void vrfObjectMessageCallback(const DtVrfObjectMessage* objMsg, void* usr);
	virtual void processVrfObjectMessage(const DtVrfObjectMessage* objectMsg);
	static void globalEnvironmentCreatedCb(const DtString& name, const DtEntityIdentifier& id, const DtUUID& uuid, void* usr);
	virtual void processGlobalEnvironmentCreated(const DtString& name, const DtEntityIdentifier& id, const DtUUID& uuid);
public:
	virtual bool newScenario(
		const DtFilename& dbname,
		const DtFilename& guidbname,
		const DtFilename& opdname,
		const DtFilename& physicalWorldName,
		const std::vector<DtString>& simulationModelSetFiles,
		const DtScenario* baseScenario = 0,
		time_t startTimeOfDay = 12 * 3600,
		DtString* errorString = 0);
protected:
	DtExerciseConn* myExConn;
	DtReflectedEnvironmentProcessList* myReflectedEnvironmentProcessList;
	bool myCreatedReflectedEnvironmentProcessList;
	DtVrfMessageInterface* myMsgIf;
	DtVrfBackendListener* myListener;
	DtCompressedFileTransporter* myTransporter;
	DtList myReceivedFiles;
	DtList myReceivedSaves;
	DtU32 myCurrentRequestId;
	DtTime myLastCommandTime;
	DtTime myMaxWaitLoadTime;
	DtTime myMaxWaitSaveTime;
	DtList myBeList;
	DtList myScenLoadedCbs;
	DtList myBeLoadedCbs;
	DtList myScenSavedOldCbs;
	DtList myScenSavedCbs;
	DtList myScenExportedCbs;
	DtList myScenSavedCheckpointCbs;
	DtList myBeSavedCbs;
	typedef std::list<PlanCbInfo> DtPlanCallbackListType;
	DtPlanCallbackListType myPlanCbs;
	typedef std::list<StmtCbInfo> DtStatementCallbackListType;
	DtStatementCallbackListType myStmtCbs;
	typedef std::list<PlanCompleteCbInfo> DtPlanCompleteCallbaclListType;
	DtPlanCompleteCallbaclListType myPlanCompleteCbs;
	DtHashlist myObjectCbs;
	typedef std::list<std::pair<DtObjectConsoleMessageCallbackFcn, void*> >
		DtObjectConsoleMessageCallbackListType;
	DtObjectConsoleMessageCallbackListType myObjectConsoleMessageCallbackList;
	typedef std::list<std::pair<DtBackendConsoleMessageCallbackFcn, void*> >
		DtBackendConsoleMessageCallbackListType;
	DtBackendConsoleMessageCallbackListType myBackendConsoleMessageCallbackList;
	bool myCreatedMsgIf;
	DtFilename mySaveName;
	double mySimTimeForCheckpointSave;
	DtFilename myOverlayFilename;
	DtFilename mySelectionGroupsFilename;
	DtFilename mySymbolMapFilename;
	DtFilename myCreateMenuFilename;
	DtFilename myOpdName;
	DtFilename myHostilityFilename;
	DtFilename myScenarioExtrasFilename;
	DtScenario* mySaveScen;
	DtScenario* myLoadScen;
	DtVrfConfigurationFileComposer* myComposer;
	bool myRemapped;
	bool myPreserveMappings;
	bool myDidRemap;
	bool myNeedsGlobalEnvironment;
	DtDefaultVrfObjectMessageFactory myVrfObjectMsgFact;
	DtDefaultCondExprFactory myCondExprFact;
	DtDefaultStatementFactory myStmtFact;
	DtDefaultSimReportFactory myReportFact;
	DtDefaultSimTaskFactory myTaskFact;
	DtDefaultSetDataRequestFactory mySetDataFact;
	DtDefaultSimCommandFactory mySimCommandFact;
	DtDefaultAdminContentFactory myAdminContentFact;
	DtString myGuiTerrainDatabaseFilename;
	DtRwKeyValueList myScenarioData;
	DtFilename myTempDirectory;
	DtInetAddr myInetAddr;
	mutable unsigned int myRequestIdCounter;
	DtVrfObjectMessageExecutive myObjectMessageExecutive;
	DtRadioMessageListener* myRadioMessageListener;
	DtRemoteEnvironmentController* myRemoteEnvironmentController;
	int myStartTimeOfDay;
	int myScenarioExportRequestId;
	DtFilename myExportFile;
	bool myReceivedExportFile;
	bool myAwaitingExportComplete;
	bool myUseDayNightModel;
	DtDefaultSimulationModelSetValidatorManager mySimulationModelSetValidator;
	DtUUIDNetworkManager* myUUIDNetworkManager;
	typedef std::map<DtString, std::pair<DtJoystickControlResponseFcn, void*> > JoystickResponseCallbackInfo;
	JoystickResponseCallbackInfo myJoystickResponseCallbackInfo;
};

class DT_DLL_vrfcontrol DtVrfOverlayObjectModify
{
public:
	DtVrfOverlayObjectModify(int color, const DtUUID& attachTo,
		char* userData,
		int iUserDataSize, DtOverlayCreatedCallbackFcn fcn,
		DtVrfRemoteController* control);
	virtual ~DtVrfOverlayObjectModify();
	virtual int color() const;
	virtual const DtUUID& attachTo() const;
	virtual void userData(char** data, int* userDataSize);
	virtual DtVrfRemoteController* remoteController();
	virtual DtOverlayCreatedCallbackFcn userCallback();
protected:
	int myColor;
	DtUUID myAttachTo;
	DtVrfRemoteController* myController;
	char* myUserData;
	int myUserDataSize;
	DtOverlayCreatedCallbackFcn myUserCallback;
};

class DT_DLL_vrfcontrol DtVrfIndirectArtilleryObjectModify
{
public:
	DtVrfIndirectArtilleryObjectModify(DtVrfRemoteController* control);
	virtual ~DtVrfIndirectArtilleryObjectModify();
	virtual DtVrfRemoteController* remoteController() const;
	virtual void setRemoteController(DtVrfRemoteController*);
	virtual const DtIfIndirectArtilleryData & data() const;
	virtual DtIfIndirectArtilleryData & data();
	virtual void setData(const DtIfIndirectArtilleryData & data);
protected:
	DtVrfRemoteController* myController;
	DtIfIndirectArtilleryData myData;
};