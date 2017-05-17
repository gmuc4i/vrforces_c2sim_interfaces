#include "stdafx.h"
#include "WSS_Process.h"
#include "xerces_utils.h"


/************************
*	parseIBMLOrder   	*
************************/
char* parseIBMLOrder(
	wstring submitter,
	CWISEDriver* thisObject,
	OrderData* order,
	IWISEDriverSink* sink,
	WISE_HANDLE hDB,
	DOMDocument* domDoc)
{
	DOMElement* root;
	ReportData* report = NULL;
	int numResults = 0,
		numTasks = 0,
		groundTasks = 0,
		airTasks = 0,
		numAffectedWho = 0,
		numReports = 0,
		numTaskControlMeasures = 0,
		numOrderControlMeasures;
	int taskNum, taskControlMeasureNum, orderControlMeasureNum, affectedWhoNum; //iterators
	DOMXPathResult* taskResult = NULL;
	DOMXPathResult* groundResult = NULL;
	DOMXPathResult* airResult = NULL;
	DOMXPathResult* taskControlMeasureResult = NULL;
	DOMXPathResult* orderControlMeasureResult = NULL;
	DOMXPathResult* taskOrgResult = NULL;
	DOMXPathResult* nextTaskOrgResult = NULL;
	DOMXPathResult* affectedWhoResult = NULL;
	DOMXPathResult* reportResult = NULL;
	DOMXPathResult* executerResult = NULL;
	DOMXPathResult* result = NULL;
	DOMElement* taskElement;
	DOMElement* taskControlMeasureElement;
	DOMElement* orderControlMeasureElement;
	DOMElement* taskWhereElement;
	DOMElement* affectedWhoElement;
	DOMElement* topTaskOrgElement;
	ReportDetail* reportDetail = NULL;
	wstring taskType = L"G";
	char* testError = NULL;

	// Get the top level node
	root = domDoc->getDocumentElement();
	if (findNode(domDoc, root, L"./OrderPush", result))
	{
		root = static_cast <DOMElement*> (result->getNodeValue());
		/*
		Parse Order

		<xsd:complexType name="OrderType">
		<xsd:sequence>
		<xsd:element name="OrderIssuedWhen" type="mip:Datetime18XmlType" />
		<xsd:element name="OrderID" type="LabelType" />
		<!-- not parsed -->
		<xsd:element name="ReferenceOrderID" type="LabelType" minOccurs="0" />
		<!-- not parsed -->
		<xsd:element name="Context" type="LabelType" minOccurs="0" />
		<xsd:element name="TaskerWho" type="WhoType" />
		<!-- not parsed -->
		<xsd:element name="Situation" maxOccurs="1" minOccurs="0">
		<xsd:complexType>
		<xsd:sequence>
		<xsd:element name="FriendInitialSituation" type="SituationType" minOccurs="0" />
		<xsd:element name="EnnemyInitialSituation" type="SituationType" minOccurs="0" />
		<xsd:element name="EnnemyHypothesis" type="EnnemyHypothesisOrderType" minOccurs="0" maxOccurs="unbounded" />
		</xsd:sequence>
		</xsd:complexType>
		</xsd:element>
		<xsd:element name="Execution" maxOccurs="1" minOccurs="1">
		<xsd:complexType>
		<xsd:sequence>
		<xsd:element name="TaskersIntent" type="FreeTextType" minOccurs="0" />
		<!-- Not Implemented -->
		<xsd:element name="Phase" type="PhaseType" minOccurs="0"/>
		<xsd:element name="Tasks" type="TasksType" />
		<xsd:element name="ControlMeasures" type="OrderControlMeasuresType" minOccurs="0" />
		</xsd:sequence>
		</xsd:complexType>
		</xsd:element>
		<xsd:element name="TaskOrganization" type="TaskOrganizationType" />
		</xsd:sequence>
		</xsd:complexType>
		*/
		// OrderIssued When
		if (!findAndAssign(domDoc, root, L"./OrderIssuedWhen", &(order->c_OrderIssuedWhen)))
			return printError(submitter, thisObject, "missing OrderIssuedWhen");

		// Order ID
		if (!findAndAssign(domDoc, root, L"./OrderID", &(order->c_OrderID)))
			return printError(submitter, thisObject, "missing OrderID");

		// TaskerWho
		if (!findAndAssign(domDoc, root, L"./TaskerWho/UnitID", &(order->c_TaskerWho)))
			return printError(submitter, thisObject, "missing TaskerWho/UnitID");
		/*
		<xsd:complexType name="TasksType">
		<xsd:sequence>
		<xsd:element name="Task" type="TaskType" minOccurs="1" maxOccurs="unbounded"/>
		</xsd:sequence>
		</xsd:complexType>
		*/
		// Look for multiple tasks; accept
		// previous GroundTask node (not really IBML09)
		groundTasks = findNode(domDoc, root, L"./Tasks/GroundTask", groundResult);
		if (groundTasks == 0)// Adam hack
			groundTasks = findNode(domDoc, root, L"./TaskList/GroundTask", groundResult);
		airTasks = findNode(domDoc, root, L"./Tasks/AirTask", airResult);
		if (airTasks == 0)// Adam hack
			airTasks = findNode(domDoc, root, L"./TaskList/AirTask", airResult);
		if (airTasks == 0)// Adam hack
			airTasks = findNode(domDoc, root, L"./TaskList/Task", airResult);
		numTasks = airTasks + groundTasks;
		if (numTasks == 0)
			numTasks = findNode(domDoc, root, L"./Task", groundResult);
		if (numTasks == 0)// Adam hack
			numTasks = findNode(domDoc, root, L"./TaskList", groundResult);
		if (numTasks == 0)return printError(submitter, thisObject, "missing Task or TaskList");

		// Loop through repeated Task entries
		for (taskNum = 0; taskNum < numTasks; taskNum++)
		{
			/*
			<xsd:complexType name="TaskType">
			<xsd:sequence>
			<xsd:element name="TaskeeWho" type="WhoType"/>
			<xsd:element name="What" type="WhatType"/>
			<xsd:element name="Where" type="WhereType" minOccurs="0"/>
			<xsd:element name="StartWhen" type="WhenType"/>
			<xsd:element name="EndWhen" type="WhenType" minOccurs="0"/>
			<xsd:element name="AffectedWho" type="ObservedWhoType" minOccurs="0" maxOccurs="unbounded"/>
			<xsd:element name="Why" type="WhyType" minOccurs="0"/>
			<xsd:element name="Priority" type="mip:ActionTaskPriorityCodeXmlType" minOccurs="0"/>
			<xsd:element name="TaskControlMeasures" type="TaskControlMeasuresType" minOccurs="0"/>
			<xsd:element name="TaskID" type="LabelType"/>
			</xsd:sequence>
			</xsd:complexType>
			*/
			if (taskNum >= airTasks) // ground tasks
			{
				//get the task from the list and cast it to a DOMElement
				taskType = L"G";
				if (groundTasks == 0)
					findNode(domDoc, root, L"./Task", groundResult);
				else
					findNode(domDoc, root, L"./Task/GroundTask", groundResult);
				groundResult->snapshotItem(taskNum - airTasks);
				taskElement = static_cast <DOMElement*> (groundResult->getNodeValue());
			}
			else
			{
				//get the task from the list and cast it to a DOMElement
				taskType = L"A";
				airResult->snapshotItem(taskNum);
				taskElement = static_cast <DOMElement*> (airResult->getNodeValue());
			}

			// Allocate new TaskData struct and push it on Vector in OrderData
			struct TaskData *task = new(struct TaskData);
			order->c_tasks.push_back(task);

			// TaskType: Ground or Air
			task->c_TaskType = taskType;
			/*
			from TaskType:
			<xsd:element name="TaskeeWho" type="WhoType"/>
			*/
			// TaskeeWho
			if (!findAndAssign(domDoc, taskElement, L"./TaskeeWho/UnitID", &(task->c_TaskeeWho)))
				return printError(submitter, thisObject,
					"missing TaskeeWho/UnitID for task number ",
					intToString(taskNum));
			/*
			from TaskType:
			<xsd:element name="What" type="WhatType"/>

			<xs:complexType name="WhatType">
			<xs:sequence>
			<xs:element name="WhatCode" type="mip:ActionTaskActivityCodeXmlType"/>
			</xs:sequence>
			</xs:complexType>
			*/
			// Task What
			if (!findAndAssign(domDoc, taskElement, L"./What/WhatCode", &(task->c_ActionCode)))
				return printError(submitter, thisObject, "missing WhatCode");
			/*
			from TaskType:
			<xsd:element name="Where" type="WhereType" />
			*/
			// Task Where
			if (findNode(domDoc, taskElement, L"./Where", groundResult))
			{
				// make a new ControlMeasure to hold Task/Where
				ControlMeasure *taskControlMeasure = new ControlMeasure;
				taskControlMeasure->c_RouteWhereAlongTowardsFromLocation = NULL;
				taskControlMeasure->c_RouteWhereToLocation = NULL;
				task->c_TaskWhere = taskControlMeasure;

				// parse the Where
				taskWhereElement = static_cast <DOMElement*> (groundResult->getNodeValue());
				char* whereResult = parseIBMLWhere(submitter, thisObject, domDoc, taskWhereElement, taskControlMeasure, task, NULL);
				if (whereResult)return whereResult;
			}
			else printError(submitter, thisObject, "missing Where in Task");
			/*
			from TaskType:
			<xsd:element name="StartWhen" type="WhenType"/>
			<xsd:element name="EndWhen" type="WhenType" minOccurs="0"/>

			<xs:complexType name="WhenType">
			<xs:choice>
			<xs:element name="WhenTime" type="WhenTimeType"/>
			<xs:element name="RelativeWhen" type="RelativeWhenType"/>
			</xs:choice>
			</xs:complexType>

			<xs:complexType name="WhenTimeType">
			<xs:sequence>
			<xs:choice>
			<xs:element name="StartTimeQualifier" type="mip:ActionTaskStartQualifierCodeXmlType"/>
			<xs:element name="EndTimeQualifier" type="mip:ActionTaskEndQualifierCodeXmlType"/>
			</xs:choice>
			<xs:element name="DateTime" type="mip:Datetime18XmlType"/>
			</xs:sequence>
			</xs:complexType>

			<xs:complexType name="RelativeWhenType">
			<xs:sequence>
			<xs:element name="Qualifier" type="mip:ActionTemporalAssociationCategoryCodeXmlType"/>
			<xs:element name="ReferenceDuration" type="mip:Duration19XmlType" minOccurs="0">
			<xs:annotation>
			<xs:documentation>The format of ReferenceDuration is the same as a Date ((+/-)YYYYMMDDHHMMSS.SSS)
			but remember it's a Duration not a Date. For instance, If you want to start a task no later
			than 1hour 2min 30sec after the end of another task you would set Qualifier to STRENL and
			ReferenceDuration to +00000000010230.000.</xs:documentation>
			</xs:annotation>
			</xs:element>
			<xs:element name="TaskID" type="LabelType" minOccurs="0"/>
			<xs:element name="RelativeWhenReferenceTaskID" type="LabelType" minOccurs="0"/>
			</xs:sequence>
			</xs:complexType>
			*/
			// Task/StartWhen (choice of absolute or relative; all but DateTime and Qualifier optional)
			if (findNode(domDoc, taskElement, L"./StartWhen/WhenTime/DateTime", result))
			{
				if (!findAndAssign(domDoc, taskElement, L"./StartWhen/WhenTime/StartTimeQualifier", &(task->c_StartQualifier)))
					return printError(submitter, thisObject, "missing StartWhen/WhenTime/StartTimeQualifier");
				if (!findAndAssign(domDoc, taskElement, L"./StartWhen/WhenTime/DateTime", &(task->c_StartTime)))
					return printError(submitter, thisObject, "missing StartWhen/WhenTime/DateTime");
			}
			else
			{
				if (!findAndAssign(domDoc, taskElement, L"./StartWhen/RelativeWhen/Qualifier", &(task->c_StartQualifier)))
					return printError(submitter, thisObject, "missing StartWhen/RelativeWhen/Qualifier");
				findAndAssign(domDoc, taskElement, L"./StartWhen/RelativeWhen/ReferenceDuration",
					&(task->c_StartReferenceDuration));
				findAndAssign(domDoc, taskElement, L"./StartWhen/RelativeWhen/TaskID", &(task->c_StartWhenTask));
				findAndAssign(domDoc, taskElement, L"./StartWhen/RelativeWhen/RelativeWhenReferenceTaskID",
					&(task->c_RelativeWhenStartReferenceTaskID));
			}

			// Task End Time (absolute or relative; option but if present must have DateTime and Qualifier)
			if (findNode(domDoc, taskElement, L".//EndWhen/WhenTime/EndTimeQualifier", result))
			{
				if (!findAndAssign(domDoc, taskElement, L"./EndWhen/WhenTime/EndTimeQualifier", &(task->c_EndQualifier)))
					return printError(submitter, thisObject, "missing EndWhen/WhenTime/EndTimeQualifier");
				if (!findAndAssign(domDoc, taskElement, L"./EndWhen/WhenTime/DateTime", &(task->c_EndTime)))
					return printError(submitter, thisObject, "EndWhen/WhenTime/DateTime");
			}
			else
			{
				if (findNode(domDoc, taskElement, L"./EndWhen/RelativeWhen/Qualifier", result))
				{
					if (!findAndAssign(domDoc, taskElement, L"./EndWhen/RelativeWhen/Qualifier", &(task->c_EndQualifier)))
						return printError(submitter, thisObject, "missing EndWhen/RelativeWhen/Qualifier");
					findAndAssign(domDoc, taskElement, L"./EndWhen/RelativeWhen/ReferenceDuration",
						&(task->c_EndReferenceDuration));
					findAndAssign(domDoc, taskElement, L"./EndWhen/RelativeWhen/TaskID", &(task->c_EndWhenTask));
					findAndAssign(domDoc, taskElement, L"./EndWhen/RelativeWhen/RelativeWhenReferenceTaskID",
						&(task->c_RelativeWhenEndReferenceTaskID));
				}
			}
			/*
			from TaskType:
			<xsd:element name="AffectedWho" type="ObservedWhoType" minOccurs="0" maxOccurs="unbounded"/>

			<xs:complexType name="ObservedWhoType">
			<xs:choice>
			<xs:element name="UnitID" type="LabelType"></xs:element>
			<xs:element name="UnitDescription">
			<xs:complexType>
			<xs:sequence>
			<xs:element name="Hostility" type="mip:ObjectItemHostilityStatusCodeXmlType"/>
			<xs:element name="Size" type="mip:UnitTypeSizeCodeXmlType"/>
			<xs:element name="ArmCategory" type="mip:UnitTypeArmCategoryCodeXmlType"/>
			<xs:element name="Qualifier" type="mip:UnitTypeQualifierCodeXmlType" minOccurs="0"/>
			</xs:sequence>
			</xs:complexType>
			</xs:element>
			</xs:choice>
			</xs:complexType>
			*/
			// Task AffectedWho (optional) may be more than one
			int numAffectedWho = findNode(domDoc, taskElement, L"./AffectedWho", affectedWhoResult);
			if (numAffectedWho > 0)
			{
				for (affectedWhoNum = 0; affectedWhoNum < numAffectedWho; affectedWhoNum++)
				{
					// make another ObservedWho and push in onto the AffectedWho vector
					ObservedWho *affectedWhoObserved = new(ObservedWho);
					task->c_AffectedWhoVector.push_back(affectedWhoObserved);

					//get the report from the list and cast it to a DOMElement
					affectedWhoResult->snapshotItem(affectedWhoNum);
					affectedWhoElement = static_cast <DOMElement*> (affectedWhoResult->getNodeValue());

					// parse it
					if (parseIBMLObservedWho(submitter, thisObject, domDoc, affectedWhoElement, affectedWhoObserved, task))
						return printTaskError(submitter, thisObject, "error in AffectedWho", task);

				}//end AffectedWho loop
			}// end if(numAffectedWho > 0)
			 /*
			 from TaskType:
			 <xsd:element name="Why" type="WhyType" minOccurs="0"/>

			 <xs:complexType name="WhyType">
			 <xs:sequence>
			 <xs:element name="Effect" type="mip:ActionEffectDescriptionCodeXmlType" minOccurs="0">
			 </xs:element>
			 </xs:sequence>
			 </xs:complexType>
			 */
			 // Why (optional)
			if (findAndAssign(domDoc, taskElement, L"./Why/Effect", &(task->c_WhyEffect)))
				if ((testError = printNoCBML(submitter, thisObject, "Why/Effect")))
					return testError;
			/*
			from TaskType:
			<xsd:element name="Priority" type="mip:ActionTaskPriorityCodeXmlType" minOccurs="0"/>
			*/
			// Priority (optional)
			if (findAndAssign(domDoc, taskElement, L"./Priority", &(task->c_Priority)))
				if ((testError = printNoCBML(submitter, thisObject, "Task/Priority")))
					return testError;
			/*
			from taskType:

			<xsd:element name="TaskControlMeasures" type="TaskControlMeasuresType" minOccurs="0">
			</xsd:element>

			<xsd:complexType name="TaskControlMeasuresType">
			<xsd:sequence>
			<xsd:element name="ControlMeasureLabel" type="LabelType" minOccurs="0" maxOccurs="unbounded"/>
			</xsd:sequence>
			</xsd:complexType>
			*/
			// enter any number of TaskControlMeasures into database (these hold labels of OrderControlMeasures)
			numTaskControlMeasures = findNode(domDoc, taskElement, L"//ControlMeasureLabel", taskControlMeasureResult);
			for (taskControlMeasureNum = 0;
				taskControlMeasureNum < numTaskControlMeasures;
				++taskControlMeasureNum)
			{
				// make a TaskControlMeasure and push it onto the database taskControlMeasures vector
				taskControlMeasureResult->snapshotItem(taskControlMeasureNum);
				taskControlMeasureElement = static_cast <DOMElement*> (taskControlMeasureResult->getNodeValue());
				struct TaskControlMeasure *taskControlMeasure = new(struct TaskControlMeasure);
				task->c_taskControlMeasures.push_back(taskControlMeasure);
				findAndAssign(domDoc, taskControlMeasureElement, L".//ControlMeasureLabel",
					&(taskControlMeasure->c_TaskControlMeasureLabel));
			}

			// parse TaskID
			if (!findAndAssign(domDoc, taskElement, L"./TaskID", &(task->c_TaskID)))
				return printError(submitter, thisObject, "missing TaskID");

		}// end Task loop

		 /*
		 from OrderType:
		 <xsd:element name="ControlMeasures" type="OrderControlMeasuresType" minOccurs="0" />

		 <xsd:complexType name="OrderControlMeasuresType">
		 <xsd:sequence>
		 <xsd:element name="ControlMeasure" type="WhereType" maxOccurs="unbounded"/>
		 </xsd:sequence>
		 </xsd:complexType>
		 <xsd:annotation>
		 */
		 // parse OrderControlMeasures
		numOrderControlMeasures = findNode(domDoc, root, L"./ControlMeasures/ControlMeasure",
			orderControlMeasureResult);
		for (orderControlMeasureNum = 0;
			orderControlMeasureNum < numOrderControlMeasures;
			++orderControlMeasureNum)
		{
			// make an OrderControlMeasure and push it onto the database orderControlMeasures vector
			orderControlMeasureResult->snapshotItem(orderControlMeasureNum);
			orderControlMeasureElement = static_cast <DOMElement*> (orderControlMeasureResult->getNodeValue());
			ControlMeasure *orderControlMeasure = new ControlMeasure;
			orderControlMeasure->c_RouteWhereAlongTowardsFromLocation = NULL;
			orderControlMeasure->c_RouteWhereToLocation = NULL;
			order->c_orderControlMeasures.push_back(orderControlMeasure);
			if ((testError = parseIBMLWhere(
				submitter,
				thisObject,
				domDoc,
				orderControlMeasureElement,
				orderControlMeasure,
				NULL,
				order)))
				return testError;
		}
		/*
		from OrderType:
		<xsd:element name="TaskOrganization" type="TaskOrganizationType" />

		<xsd:complexType name="TaskOrganizationType">
		<xsd:annotation>
		<xsd:documentation>Defines the task organisation associated with the order</xsd:documentation>
		</xsd:annotation>
		<xsd:sequence>
		<xsd:element name="UnitID" type="LabelType"/>
		<!-- Task Organization Unit's position and status will be provided by another SBML service. -->
		<!-- <xsd:element name="WhereLocation" type="WhereLocationType" minOccurs="0"/> -->
		<xsd:element name="TaskOrganization" type="TaskOrganizationType" minOccurs="0" maxOccurs="unbounded"/>
		</xsd:sequence>
		</xsd:complexType>
		*/
		// parse and linearize TaskOrganization (a tree of indeterminate depth)
		if (findNode(domDoc, root, L"./TaskOrganization", taskOrgResult))
		{
			topTaskOrgElement = static_cast <DOMElement*> (taskOrgResult->getNodeValue());
			TaskOrg *taskOrg = new(TaskOrg);
			order->c_TaskOrgVector.push_back(taskOrg);
			if (parseIBMLTaskOrg(submitter, thisObject, domDoc, order, taskOrg, topTaskOrgElement, L"") < 0)
				return printError(submitter, thisObject, "incorrect TaskOrganization");
		}


	}// end Order parsing

	if (taskResult != NULL)
		delete taskResult;
	if (groundResult != NULL)
		delete groundResult;
	if (airResult != NULL)
		delete airResult;
	if (taskControlMeasureResult != NULL)
		delete taskControlMeasureResult;
	if (orderControlMeasureResult != NULL)
		delete orderControlMeasureResult;
	if (taskOrgResult != NULL)
		delete taskOrgResult;
	if (nextTaskOrgResult != NULL)
		delete nextTaskOrgResult;
	if (affectedWhoResult != NULL)
		delete affectedWhoResult;
	if (reportResult != NULL)
		delete reportResult;
	if (executerResult != NULL)
		delete executerResult;
	if (result != NULL)
		delete result;

	return NULL;

}// end parseIBMLOrder()



 /************************
 *	parseIBML_Report	*
 ************************/
 // Extract the data elements needed to generate other report types

char* parseIBMLReport(
	wstring submitter,
	CWISEDriver* thisObject,
	ReportData* report,
	IWISEDriverSink* sink,
	WISE_HANDLE hDB,
	DOMDocument* domDoc)
{
	/*
	<xs:element name="BMLReport" type="BMLReportType"/>
	<xs:complexType name="BMLReportType">
	<xs:sequence>
	<xs:element name="Report" maxOccurs="unbounded">
	<xs:complexType>
	<xs:sequence>
	<xs:element name="CategoryOfReport" type="CategoryOfReportType"/>
	<xs:element name="TypeOfReport" type="TypeOfReportType"/>
	<xs:choice>
	<xs:element name="StatusReport" type="StatusReportsType"/>
	</xs:choice>
	</xs:sequence>
	</xs:complexType>
	</xs:element>
	</xs:sequence>
	</xs:complexType>
	<xs:simpleType name="CategoryOfReportType">
	<xs:restriction base="xs:string">
	<xs:enumeration value="StatusReport"/>
	</xs:restriction>
	</xs:simpleType>
	<xs:simpleType name="TypeOfReportType">
	<xs:restriction base="xs:string">
	<xs:enumeration value="GeneralStatusReport"/>
	<xs:enumeration value="TaskStatusReport"/>
	<xs:enumeration value="WhoHoldingStatusReport"/>
	<xs:enumeration value="ControlFeatureReport"/>
	<xs:enumeration value="FacilityStatusReport"/>
	</xs:restriction>
	</xs:simpleType>
	*/
	DOMElement* root;
	int numResults = 0, numReports = 0;
	DOMXPathResult* reportNodeResult = NULL;
	DOMXPathResult* reportResult = NULL;
	DOMXPathResult* result = NULL;
	DOMXPathResult* gdcResult = NULL;
	DOMElement* reportNodeElement;
	DOMElement* reportElement;
	char* testError = NULL;
	char* errorText = NULL;

	// Get the top level node
	root = domDoc->getDocumentElement();

	// Look for what may be multiple reports
	numReports = findNode(domDoc, root, L"/BMLReport/Report", reportNodeResult);

	// Loop through repeated Report entries
	for (int reportNum = 0; reportNum < numReports; reportNum++)
	{
		//get the report from the list and cast it to a DOMElement
		reportNodeResult->snapshotItem(reportNum);
		reportNodeElement = static_cast <DOMElement*> (reportNodeResult->getNodeValue());

		// Instantiate struct to hold data on this report and add it to vector of reports
		struct ReportDetail* reportDetail = new (struct ReportDetail);
		report->c_details.push_back(reportDetail);

		// parse ReportType
		if (!findAndAssign(
			domDoc,
			reportNodeElement,
			L"./TypeOfReport",
			&(reportDetail->c_Report_Type)))
			return printError(submitter, thisObject, "missing Report/TypeOfReport");

		// locate reportElement under tag = TypeOfReport
		if (!findNode(
			domDoc,
			reportNodeElement,
			L"./StatusReport/" + reportDetail->c_Report_Type,
			reportResult))
			return printError(submitter, thisObject, "missing Report/StatusReport/", reportDetail->c_Report_Type);
		reportElement = static_cast <DOMElement*> (reportResult->getNodeValue());

		// ToDo: implement RelativeReportedWhen
		/*
		IBML has no SpotReport
		<xs:complexType name="SpotReportType">
		<xs:sequence>
		<xs:element name="Hostility" type="jc3iedm:ObjectItemHostilityStatusCode" minOccurs="0"/>
		<xs:element name="Executer" type="jc3iedm:OIDType" minOccurs="0"/>
		<xs:element name="AtWhere" type="cbml:PointLightType" minOccurs="0"/>
		<xs:element name="Context" type="jc3iedm:TextTypeVar80" minOccurs="0"/>
		<xs:element name="Parameters">
		<xs:complexType>
		<xs:sequence>
		<xs:element name="Size" type="xs:string" minOccurs="0"/>
		<xs:element name="Activity" type="xs:string" minOccurs="0"/>
		<xs:element name="Location" type="cbml:PointLightType" minOccurs="0"/>
		<xs:element name="Unit" type="jc3iedm:OIDType" minOccurs="0"/>
		<xs:element name="Equipment" type="jc3iedm:VehicleTypeCategoryCode"
		minOccurs="0"/>
		<xs:element name="SendersAssesment" type="xs:string" minOccurs="0"/>
		<xs:element name="Narrative" type="xs:string" minOccurs="0"/>
		<xs:element name="Authentication" type="xs:string" minOccurs="0"/>
		</xs:sequence>
		</xs:complexType>
		</xs:element>
		</xs:sequence>
		</xs:complexType>

		// parse Spot Report
		if(reportDetail->c_Report_Type == L"ObserveTaskReport")
		{
		// Context (optional)
		findAndAssign(domDoc, reportElement, L"./Context", &(reportDetail->c_Context));

		// Hostility (optional)
		findAndAssign(domDoc, reportElement, L"./Hostility", &(reportDetail->c_HostilityCode));

		// Executer (optional)
		if(!findAndAssign(domDoc, reportElement, L"./Executer/OID", &(reportDetail->c_ExecuterUnitID)))
		if(!findAndAssign(domDoc, reportElement, L"./Executer", &(reportDetail->c_ExecuterUnitID)))// OneSAF hack
		return printError(submitter, thisObject, "SpotReport missing Executer/OID");
		if(reportDetail->c_Reporter == L"")
		reportDetail->c_Reporter = reportDetail->c_ExecuterUnitID;
		reportDetail->c_ExecuterParametersUnitID = reportDetail->c_ExecuterUnitID;

		// AtWhere (optional)
		// Where elements (ElevationAGL optional)
		// locate the SpecificPoint of Executer
		Point* gdcPoint = new Point;
		if(findNode(domDoc, reportElement, L"./AtWhere/GDC", &gdcResult))
		{
		gdcResult->snapshotItem(0);
		DOMElement* gdcElement = static_cast <DOMElement*> (gdcResult->getNodeValue());

		// parse the GDC and place results into reportDetail
		if((testError = parseIBMLPoint(submitter, thisObject, domDoc, gdcElement, gdcPoint, "AtWhere/GDC")))
		return testError;
		reportDetail->c_LocationID = L"";// ToDo: ganerate an ID??
		reportDetail->c_Location_Latitude = gdcPoint->c_Latitude;
		reportDetail->c_Location_Longitude = gdcPoint->c_Longitude;
		reportDetail->c_Location_Elevation = gdcPoint->c_ElevationAGL;
		delete gdcPoint;
		}

		// Parameters:

		// Size (optional)
		findAndAssign(domDoc, reportElement, L"./Parameters/Size", &(reportDetail->c_ExecuterSize));

		// Activity (optional)
		findAndAssign(domDoc, reportElement, L"./Parameters/Activity", &(reportDetail->c_ExecuterActivity));

		// Location (optional)
		// Where elements (ElevationAGL optional)
		// locate the SpecificPoint of Unit observed
		gdcPoint = new Point;
		if(findNode(domDoc, reportElement, L"./Parameters/Location/GDC", &gdcResult))
		{
		gdcResult->snapshotItem(0);
		DOMElement* gdcElement = static_cast <DOMElement*> (gdcResult->getNodeValue());

		// parse the GDC and place results into reportDetail
		if((testError = parseIBMLPoint(
		submitter,
		thisObject,
		domDoc,
		gdcElement,
		gdcPoint,
		"Parameters/Location/GDC")))
		return testError;
		reportDetail->c_Spot_LocationID = L"";// ToDo: ganerate an ID??
		reportDetail->c_Spot_Latitude = gdcPoint->c_Latitude;
		reportDetail->c_Spot_Longitude = gdcPoint->c_Longitude;
		reportDetail->c_Spot_Elevation = gdcPoint->c_ElevationAGL;
		delete gdcPoint;
		}

		// Unit (optional)
		findAndAssign(submitter,
		domDoc,
		reportElement,
		"./Parameters/Unit",
		&(reportDetail->c_ExecuterUnitID));//debugx c_ExecuterParametersUnitID

		// Equipment (optional)
		findAndAssign(domDoc, reportElement, L"./Parameters/Equipment", &(reportDetail->c_ExecuterEquipment));

		// SendersAssesment (optional)
		findAndAssign(domDoc, reportElement, L"./Parameters/SendersAssesment", &(reportDetail->c_SendersAssesment));

		// Equipment (optional)
		findAndAssign(domDoc, reportElement, L"./Parameters/Narrative", &(reportDetail->c_Narrative));

		// Equipment (optional)
		findAndAssign(domDoc, reportElement, L"./Parameters/Authentication", &(reportDetail->c_Authentication));

		}	// end SpotReport
		*/
		/*
		<xs:element name="GeneralStatusReport">
		<xs:complexType>
		<xs:sequence>
		<xs:element name="ReporterWho" type="bml:WhoType"/>
		<xs:element name="Context" type="bml:LabelType" minOccurs="0"/>
		<xs:element name="Hostility" type="mip:ObjectItemHostilityStatusCodeXmlType"/>
		<xs:element name="Executer" type="bml:ObservedWhoType"/>
		<xs:element name="OpStatus" type="mip:OrganisationStatusOperationalStatusCodeXmlType"/>
		<xs:element name="WhereLocation" type="bml:WhereLocationType"/>
		<xs:element name="VelocityVector" type="bml:VelocityVectorType" minOccurs="0"/>
		<xs:element name="When" type="mip:Datetime18XmlType"/>
		<xs:element name="ReportID" type="bml:LabelType"/>
		<xs:element name="Credibility" type="bml:CredibilityType"/>
		</xs:sequence>
		</xs:complexType>
		</xs:element>

		<xs:complexType name="WhoType">
		<xs:choice>
		<xs:element name="UnitID" type="LabelType"/>
		</xs:choice>
		</xs:complexType>

		<xs:complexType name="ObservedWhoType">
		<xs:choice>
		<xs:element name="UnitID" type="LabelType"></xs:element>
		<xs:element name="UnitDescription">
		<xs:complexType>
		<xs:sequence>
		<xs:element name="Hostility" type="mip:ObjectItemHostilityStatusCodeXmlType"/>
		<xs:element name="Size" type="mip:UnitTypeSizeCodeXmlType"/>
		<xs:element name="ArmCategory" type="mip:UnitTypeArmCategoryCodeXmlType"/>
		<xs:element name="Qualifier" type="mip:UnitTypeQualifierCodeXmlType" minOccurs="0"/>
		</xs:sequence>
		</xs:complexType>
		</xs:element>
		</xs:choice>
		</xs:complexType>

		<xs:complexType name="WhereLocationType">
		<xs:complexContent>
		<xs:extension base="Coordinates">
		<xs:attribute name="Sequence" type="xs:integer"/>
		</xs:extension>
		</xs:complexContent>
		</xs:complexType>

		<xs:complexType name="Coordinates">
		<xs:choice>
		<xs:element ref="GDC"/>
		</xs:choice>
		</xs:complexType>

		<xs:element name="GDC">
		<xs:annotation>
		<xs:documentation>Geodetic coordinates in fractional degress of latitude and longitude</xs:documentation>
		</xs:annotation>
		<xs:complexType>
		<xs:all>
		<xs:element ref="Latitude"/>
		<xs:element ref="Longitude"/>
		<xs:element ref="ElevationAGL" minOccurs="0"/>
		</xs:all>
		</xs:complexType>
		</xs:element>

		<xs:complexType name="CredibilityType">
		<xs:all>
		<xs:element name="Source" type="mip:ReportingDataSourceTypeCodeXmlType"/>
		<xs:element name="Reliability" type="mip:ReportingDataReliabilityCodeXmlType"/>
		<xs:element name="Certainty" type="mip:ReportingDataCredibilityCodeXmlType"/>
		</xs:all>
		</xs:complexType>
		*/
		// parse General Status Report
		if (reportDetail->c_Report_Type == L"GeneralStatusReport")
		{
			// Report ID
			if (!findAndAssign(domDoc, reportElement, L"./ReportID", &(reportDetail->c_ReportID)))
				return printError(submitter, thisObject, "GeneralStatusReport missing ReportID");

			// Reporter
			if (!findAndAssign(domDoc, reportElement, L"./ReporterWho/UnitID", &(reportDetail->c_Reporter)))
				return printReportError(submitter, thisObject, "GeneralStatusReport missing ReporterWho/UnitID", reportDetail);

			// ReportingForceSide of ReporterWho JMP15Nov
			reportDetail->c_ReportingForceSide =
				getUnitForceSideName(
					submitter,
					thisObject,
					sink,
					hDB,
					reportDetail->c_Reporter,
					errorText);
			if (errorText != NULL)return errorText;
			if (reportDetail->c_ReportingForceSide == L"Unknown")
				printError(submitter, thisObject, "ReporterWho not in MSDL:", reportDetail->c_Reporter);

			// Context (optional)
			findAndAssign(domDoc, reportElement, L"./Context", &(reportDetail->c_Context));

			// Hostility
			if (!findAndAssign(domDoc, reportElement, L"./Hostility", &(reportDetail->c_HostilityCode)))
				return printError(submitter, thisObject, "GeneralStatusReport missing Hostility");

			// Executer (Unit being reported on)
			if (findAndAssign(domDoc, reportElement, L"./Executer/Taskee/UnitID",
				&(reportDetail->c_ExecuterUnitID)))
			{
				reportDetail->c_ExecuterParametersUnitID = reportDetail->c_ExecuterUnitID;
				reportDetail->c_ExecuterHostility = L"";
				reportDetail->c_ExecuterSize = L"";
				reportDetail->c_ExecuterArmCategory = L"";
				reportDetail->c_ExecuterQualifier = L"";

				// ForceSide info JMP15Nov
				reportDetail->c_ForceSideName =
					getUnitForceSideName(
						submitter,
						thisObject,
						sink,
						hDB,
						reportDetail->c_ExecuterUnitID,
						errorText);
				if (errorText != NULL)return errorText;
			}
			else
			{
				reportDetail->c_ExecuterUnitID = generateUID();
				if (reportDetail->c_ExecuterUnitID == L"")
					return printError(
						submitter,
						thisObject,
						"unidentified Reporter sequence exhausted - can't process report");
				reportDetail->c_ExecuterParametersUnitID = L"";
				if (!findAndAssign(domDoc, reportElement, L"./Executer/Taskee/UnitDescription/Hostility",
					&(reportDetail->c_ExecuterHostility)))
					return printError(submitter, thisObject, "GeneralStatusReport missing Executer Hostility");
				if (!findAndAssign(domDoc, reportElement, L"./Executer/Taskee/UnitDescription/Size",
					&(reportDetail->c_ExecuterSize)))
					return printError(submitter, thisObject, "GeneralStatusReport missing Executer Size");
				if (!findAndAssign(domDoc, reportElement, L"./Executer/Taskee/UnitDescription/ArmCategory",
					&(reportDetail->c_ExecuterArmCategory)))
					return printError(submitter, thisObject, "GeneralStatusReport missing Executer ArmCategory");
				findAndAssign(domDoc, reportElement, L"./Executer/Taskee/UnitDescription/Qualifier",
					&(reportDetail->c_ExecuterQualifier));
			}

			// Operational Status
			if (!findAndAssign(domDoc, reportElement, L"./OpStatus", &(reportDetail->c_OperationalStatus)))
				return printError(submitter, thisObject, "GeneralStatusReport missing OpStatus");

			// Where elements (ElevationAGL optional)
			if (!findAndAssign(domDoc, reportElement, L"./WhereLocation/GDC/Latitude",
				&(reportDetail->c_Location_Latitude)))
				return printError(submitter, thisObject, "GeneralStatusReport missing WhereLocation/GDC/Latitude");
			if (!findAndAssign(domDoc, reportElement, L"./WhereLocation/GDC/Longitude",
				&(reportDetail->c_Location_Longitude)))
				return printError(submitter, thisObject, "GeneralStatusReport missing WhereLocation/GDC/Longitude");
			if (findAndAssign(domDoc, reportElement, L"./WhereLocation/GDC/ElevationAGL",
				&(reportDetail->c_Location_Elevation)))
				reportDetail->c_Location_Elevation = L"";

			// Velocity Vector (optional)
			if (findNode(domDoc, reportElement, L"./VelocityVector", result))
			{
				if (!findAndAssign(domDoc, reportElement, L"./VelocityVector/Magnitude",
					&(reportDetail->c_Velocity_Magnitude)))
					return printError(submitter, thisObject, "GeneralStatusReport ReportVelocityVector missing Magnitude");
				if (!findAndAssign(domDoc, reportElement, L"./VelocityVector/BearingDegrees",
					&(reportDetail->c_Velocity_BearingDegrees)))
					return printError(submitter, thisObject, "GeneralStatusReport ReportVelocityVector missing BearingDegrees");
			}

			// Time
			if (!findAndAssign(domDoc, reportElement, L"./When", &(reportDetail->c_DateTime)))
				return printError(submitter, thisObject, "GeneralStatusReport missing When");

			// Credibility
			if (!findAndAssign(domDoc, reportElement, L"./Credibility/Source",
				&(reportDetail->c_CredibilitySource)))
				return printError(submitter, thisObject, "GeneralStatusReport missing Credibility/Source");
			if (!findAndAssign(domDoc, reportElement, L"./Credibility/Reliability",
				&(reportDetail->c_CredibilityReliability)))
				return printError(submitter, thisObject, "GeneralStatusReport missing Credibility/Reliability");
			if (!findAndAssign(domDoc, reportElement, L"./Credibility/Certainty",
				&(reportDetail->c_CredibilityCertainty)))
				return printError(submitter, thisObject, "GeneralStatusReport missing Credibility/Certainty");

		}	// end General Status Report
			/*
			<xs:element name="PositionStatusReport">
			<xs:complexType>
			<xs:sequence>
			<xs:element name="Hostility" type="jc3iedm:ObjectItemHostilityStatusCode"/>
			<xs:element name="Executer" type="jc3iedm:OIDType"/>
			<xs:element name="AtWhere" type="cbml:PointLightType"/>
			<xs:element name="Context" type="jc3iedm:TextTypeVar80" minOccurs="0"/>
			</xs:sequence>
			</xs:complexType>
			</xs:element>
			*/
			// parse Position Status Report
		if (reportDetail->c_Report_Type == L"PositionStatusReport")
		{
			// Context (optional)
			findAndAssign(domDoc, reportElement, L"./Context", &(reportDetail->c_Context));

			// Hostility
			if (!findAndAssign(domDoc, reportElement, L"./Hostility", &(reportDetail->c_HostilityCode)))
				return printError(submitter, thisObject, "PositionStatusReport missing Hostility");

			// Executer (Unit being reported on)
			if (!findAndAssign(domDoc, reportElement, L"./Executer/Taskee/UnitID",
				&(reportDetail->c_ExecuterUnitID)))
				return printError(submitter, thisObject, "Position Status Report missing Executer/Taskee/UID");
			reportDetail->c_ExecuterParametersUnitID = reportDetail->c_ExecuterUnitID;

			// ForceSide info JMP15Nov
			reportDetail->c_ForceSideName =
				getUnitForceSideName(
					submitter,
					thisObject,
					sink,
					hDB,
					reportDetail->c_ExecuterUnitID,
					errorText);
			if (errorText != NULL)return errorText;

			// infer ReportingForceSide from Hostility
			if (reportDetail->c_HostilityCode == L"FR")
				reportDetail->c_ReportingForceSide =
				reportDetail->c_ForceSideName;
			else if (reportDetail->c_HostilityCode == L"HO")
				reportDetail->c_ReportingForceSide =
				getOppositeForceSideName(reportDetail->c_ForceSideName);
			else reportDetail->c_ReportingForceSide = L"UNK";

			// Where elements (ElevationAGL optional)
			// locate the SpecificPoint
			Point *gdcPoint = new Point;
			if (!findNode(domDoc, reportElement, L"./WhereLocation/GDC", gdcResult))
				return printError(submitter, thisObject, "PositionStatusReport missing WhereLocation");
			gdcResult->snapshotItem(0);
			DOMElement* gdcElement = static_cast <DOMElement*> (gdcResult->getNodeValue());

			// parse the GDC and place results into reportDetail
			if ((testError = parseIBMLPoint(
				submitter,
				thisObject,
				domDoc,
				gdcElement,
				gdcPoint,
				"can't parse WhereLocation Point")))
				return testError;
			reportDetail->c_Location_Latitude = gdcPoint->c_Latitude;
			reportDetail->c_Location_Longitude = gdcPoint->c_Longitude;
			reportDetail->c_Location_Elevation = gdcPoint->c_ElevationAGL;
			delete gdcPoint;

		}	// end Position Status Report
			/*
			<xs:element name="TaskStatusReport">
			<xs:complexType>
			<xs:sequence>
			<xs:element name="ReportID" type="bml:LabelType"/>
			<xs:element name="When" type="mip:Datetime18XmlType"/>
			<xs:element name="TaskID" type="bml:LabelType"/>
			<xs:element name="Executer" type="bml:ExecuterType"/>
			<xs:element name="TaskStatus" type="mip:ActionTaskStatusProgressCodeXmlType"/>
			</xs:sequence>
			</xs:complexType>
			</xs:element>
			*/
			// parse Task Status Report
		if (reportDetail->c_Report_Type == L"TaskStatusReport")
		{
			// ReportID
			if (!findAndAssign(domDoc, reportElement, L"./ReportID", &(reportDetail->c_ReportID)))
				return printError(submitter, thisObject, "TaskStatusReport missing ReportID");

			// When
			if (!findAndAssign(domDoc, reportElement, L"./When", &(reportDetail->c_DateTime)))
				return printError(submitter, thisObject, "TaskStatusReport missing When");

			// ReportID
			if (!findAndAssign(domDoc, reportElement, L"./TaskID", &(reportDetail->c_TaskID)))
				return printError(submitter, thisObject, "TaskStatusReport missing ReportID");

			// Executer
			if (!findAndAssign(domDoc, reportElement, L"./Executer", &(reportDetail->c_ExecuterUnitID)))
				return printError(submitter, thisObject, "TaskStatusReport missing Executer");
			reportDetail->c_ExecuterParametersUnitID = reportDetail->c_ExecuterUnitID;

			// ForceSide info JMP15Nov
			reportDetail->c_ForceSideName =
				getUnitForceSideName(
					submitter,
					thisObject,
					sink,
					hDB,
					reportDetail->c_ExecuterUnitID,
					errorText);
			if (errorText != NULL)return errorText;

			// TaskStatus is always sent by the Taskee = Reporter
			reportDetail->c_ReportingForceSide = reportDetail->c_ForceSideName;

			// TaskStatus
			if (!findAndAssign(domDoc, reportElement, L"./TaskStatus", &(reportDetail->c_TaskStatus)))
				return printError(submitter, thisObject, "TaskStatusReport missing TaskStatus");
		}
		/*
		<xs:element name="WhoHoldingStatusReport">
		<xs:complexType>
		<xs:sequence>
		<xs:element name="ReporterWho" type="bml:WhoType"></xs:element>
		<xs:element name="When" type="mip:Datetime18XmlType"/>
		<xs:element name="WhoRef" type="bml:WhoType"></xs:element>
		<xs:element name="Holding" type="HoldingType"/>
		<xs:element name="Context" type="bml:LabelType" minOccurs="0"/>
		<xs:element name="ReportID" type="bml:LabelType"/>
		</xs:sequence>
		</xs:complexType>
		</xs:element>

		<xs:complexType name="WhoType">
		<xs:choice>
		<xs:element name="UnitID" type="LabelType"/>
		</xs:choice>
		</xs:complexType>

		<xs:complexType name="HoldingType">
		<xs:sequence>
		<xs:element name="NSN_Code" type="xs:string"></xs:element>
		<xs:element name="NSN_Name" type="xs:string" minOccurs="0"></xs:element>
		<xs:element name="IsEquipment" type="xs:boolean" minOccurs="0"></xs:element>
		<xs:element name="OperationalCount" type="xs:double" minOccurs="0"></xs:element>
		<xs:element name="TotalQuantity" type="xs:double" minOccurs="0"></xs:element>
		<xs:element name="OnHandQuantity" type="xs:double" minOccurs="0"></xs:element>
		<xs:element name="RequiredTotalQuantity" type="xs:double" minOccurs="0"></xs:element>
		<xs:element name="RequiredOnHandQuantity" type="xs:double" minOccurs="0"></xs:element>
		<xs:element name="RequiredCalculationMethodCode" type="xs:double" minOccurs="0"></xs:element>
		</xs:sequence>
		</xs:complexType>
		*/
		// parse WhoHolding Status Report
		if (reportDetail->c_Report_Type == L"WhoHoldingStatusReport")
		{
			// Report ID
			if (!findAndAssign(domDoc, reportElement, L"./ReportID", &(reportDetail->c_ReportID)))
				return printError(submitter, thisObject, "WhoHoldingStatusReport missing ReportID");

			// ReporterWho
			if (!findAndAssign(domDoc, reportElement, L"./ReporterWho/UnitID", &(reportDetail->c_Reporter)))
				return printReportError(submitter, thisObject, "WhoHoldingStatusReport missing ReporterWho", reportDetail);

			// ReportingForceSide of ReporterWho JMP15Nov
			reportDetail->c_ReportingForceSide =
				getUnitForceSideName(
					submitter,
					thisObject,
					sink,
					hDB,
					reportDetail->c_Reporter,
					errorText);
			if (errorText != NULL)return errorText;

			// When
			if (!findAndAssign(domDoc, reportElement, L"./When", &(reportDetail->c_DateTime)))
				return printError(submitter, thisObject, "WhoHoldingStatusReport missing When");

			// WhoRef
			if (!findAndAssign(domDoc, reportElement, L"./WhoRef/UnitID", &(reportDetail->c_WhoRef)))
				return printError(submitter, thisObject, "WhoHoldingStatusReport missing WhoRef");

			// Holding NSN_Code
			if (!findAndAssign(domDoc, reportElement, L"./Holding/NSN_Code", &(reportDetail->c_NSN_Code)))
				return printError(submitter, thisObject, "WhoHoldingStatusReport missing Holding/NSN_Code");

			// Holding NSN_Name
			findAndAssign(domDoc, reportElement, L"./Holding/NSN_Name", &(reportDetail->c_NSN_Name));

			// Holding IsEquipment
			findAndAssign(domDoc, reportElement, L"./Holding/IsEquipment", &(reportDetail->c_IsEquipment));

			// Holding OperationalCount
			findAndAssign(domDoc, reportElement, L"./Holding/OperationalCount",
				&(reportDetail->c_OperationalCount));

			// Holding TotalQuantity
			findAndAssign(domDoc, reportElement, L"./Holding/TotalQuantity",
				&(reportDetail->c_TotalQuantity));

			// Holding OnHandQuantity
			findAndAssign(domDoc, reportElement, L"./Holding/OnHandQuantity",
				&(reportDetail->c_OnHandQuantity));

			// Holding RequiredTotalQuantity
			findAndAssign(domDoc, reportElement, L"./Holding/RequiredTotalQuantity",
				&(reportDetail->c_RequiredTotalQuantity));

			// Holding RequiredOnHandQuantity
			findAndAssign(domDoc, reportElement, L"./Holding/RequiredOnHandQuantity",
				&(reportDetail->c_RequiredOnHandQuantity));

			// Holding RequiredCalculationMethodCode
			findAndAssign(domDoc, reportElement, L"./Holding/RequiredCalculationMethodCode",
				&(reportDetail->c_RequiredCalculationMethodCode));

			// Context (optional)
			findAndAssign(domDoc, reportElement, L"./Context", &(reportDetail->c_Context));

			// for WhoHolding, the Executer is the ReporterWho
			reportDetail->c_ExecuterUnitID = reportDetail->c_Reporter;

			// ForceSide info JMP15Nov
			reportDetail->c_ForceSideName =
				getUnitForceSideName(
					submitter,
					thisObject,
					sink,
					hDB,
					reportDetail->c_ExecuterUnitID,
					errorText);
			if (errorText != NULL)return errorText;
		}
		/*
		<xs:element name="ControlFeatureReport">
		<xs:complexType>
		<xs:sequence>
		<xs:element name="ReporterWho" type="bml:WhoType"/>
		<xs:element name="Context" type="bml:LabelType" minOccurs="0"/>
		<xs:element name="Hostility" type="mip:ObjectItemHostilityStatusCodeXmlType"/>
		<xs:element name="ControlFeatureID" type="bml:LabelType"/>
		<xs:element name="TypeCategoryCode" type="mip:ControlFeatureTypeCategoryCodeXmlType"/>
		<xs:element name="OpStatus" type="mip:OrganisationStatusOperationalStatusCodeXmlType"/>
		<xs:element name="WhereLocation" type="bml:AtWhereLocationType"/>
		<xs:element name="WhoRef" type="bml:WhoType" minOccurs="0" maxOccurs="unbounded">
		</xs:element>
		<xs:element name="When" type="mip:Datetime18XmlType"/>
		<xs:element name="ReportID" type="bml:LabelType"/>
		<xs:element name="Credibility" type="bml:CredibilityType"/>
		</xs:sequence>
		</xs:complexType>
		</xs:element>
		*/
		// parse ControlFeature Report
		if (reportDetail->c_Report_Type == L"ControlFeatureReport")
		{
			return printError(submitter, thisObject, "ControlFeatureReport not currently supported in IBML");
		}
		/*
		<xs:element name="FacilityStatusReport">
		<xs:complexType>
		<xs:sequence>
		<xs:element name="ReporterWho" type="bml:WhoType"/>
		<xs:element name="Context" type="bml:LabelType" minOccurs="0"/>
		<xs:element name="Hostility" type="mip:ObjectItemHostilityStatusCodeXmlType"/>
		<xs:element name="FacilityID" type="bml:LabelType"/>
		<xs:element name="TypeCategoryCode" type="mip:FacilityTypeCategoryCodeXmlType"/>
		<xs:element name="OpStatus" type="mip:FacilityStatusOperationalStatusQualifierCodeXmlType"/>
		<xs:element name="WhereLocation" type="bml:AtWhereLocationType"></xs:element>
		<xs:element name="ResponsibleWho" type="bml:WhoType" minOccurs="0" maxOccurs="unbounded"></xs:element>
		<xs:element name="When" type="mip:Datetime18XmlType"/>
		<xs:element name="ReportID" type="bml:LabelType"/>
		<xs:element name="Credibility" type="bml:CredibilityType"/>
		</xs:sequence>
		</xs:complexType>
		</xs:element>
		*/
		// parse FacilityStatusReport
		if (reportDetail->c_Report_Type == L"FacilityStatusReport")
		{
			return printError(submitter, thisObject, "FacilityStatusReport not currently supported in IBML");
		}
	}// end for(reportNum

	if (reportNodeResult != NULL)
		delete reportNodeResult;
	if (reportResult != NULL)
		delete reportResult;
	if (result != NULL)
		delete result;
	if (gdcResult != NULL)
		delete gdcResult;
	return NULL;

}	// end parseIBMLReport()


	// function to perform Where parsing in the element passed
char* parseIBMLWhere(
	wstring submitter,
	CWISEDriver* thisObject,
	DOMDocument* domDoc,
	DOMElement* whereElement,
	ControlMeasure *targetControlMeasure,
	TaskData *task,
	OrderData *order)
{
	int pointNum = 0, routeWhereNode = 0, atWhereNode = 0, viaNum = 0, numVias = 0, numPoints = 0;
	DOMXPathResult* routeWhereResult = NULL;
	DOMXPathResult* atWhereResult = NULL;
	DOMXPathResult* alongResult = NULL;
	DOMXPathResult* towardsResult = NULL;
	DOMXPathResult* fromViaToResult = NULL;
	DOMXPathResult* sequenceResult = NULL;
	DOMXPathResult* pointResult = NULL;
	DOMXPathResult* velocityVectorResult = NULL;

	char* testError;
	/*
	<xs:complexType name="WhereType">
	<xs:sequence>
	<xs:element name="WhereID" type="LabelType"/>
	<xs:choice>
	<xs:element name="RouteWhere" type="RouteWhereType"/>
	<xs:element name="AtWhere" type="AtWhereType"/>
	</xs:choice>
	</xs:sequence>
	</xs:complexType>
	*/
	// parse WhereID
	if (!findAndAssign(domDoc, whereElement, L"./WhereID", &(targetControlMeasure->c_WhereID)))
		if (task)return printError(submitter, thisObject, "missing Task/Where/WhereID");
		else return printError(submitter, thisObject, "missing ControlMeasure/WhereID");

		// must be RouteWhere or AtWhere
		routeWhereNode = findNode(domDoc, whereElement, L"./RouteWhere", routeWhereResult);
		atWhereNode = findNode(domDoc, whereElement, L"./AtWhere/JBMLAtWhere", atWhereResult);
		if (routeWhereNode + atWhereNode == 0)
			return printError(submitter, thisObject, L"missing RouteWhere or AtWhere for WhereID " + targetControlMeasure->c_WhereID);

		// parse RouteWhere
		if (routeWhereNode)
		{
			int along, towards, fromViaTo;
			targetControlMeasure->c_RouteWhereAlongTowardsFromLocation = new Point;
			targetControlMeasure->c_RouteWhereToLocation = new Point;
			/*
			<xs:complexType name="RouteWhereType">
			<xs:choice>
			<xs:element name="Along" type="LocationType"/>
			<xs:element name="Towards">
			<xs:complexType>
			<xs:choice>
			<xs:element name="Location" type="LocationType"/>
			<xs:element name="Direction" type="mip:DirectionCodeXmlType"/>
			</xs:choice>
			</xs:complexType>
			</xs:element>
			<xs:element name="From-Via-To">
			<xs:complexType>
			<xs:all>
			<xs:element name="From" type="LocationType" minOccurs="0"/>
			<xs:element name="Via" minOccurs="0">
			<xs:complexType>
			<xs:sequence maxOccurs="unbounded">
			<xs:element name="Waypoint">
			<xs:complexType>
			<xs:sequence>
			<xs:element name="Location" type="LocationType"/>
			</xs:sequence>
			<xs:attribute name="sequence" type="xs:int" use="required"/>
			</xs:complexType>
			</xs:element>
			</xs:sequence>
			</xs:complexType>
			</xs:element>
			<xs:element name="To" type="LocationType"/>
			</xs:all>
			</xs:complexType>
			</xs:element>
			</xs:choice>
			</xs:complexType>
			*/
			// options in RouteWhere: Along, Toward, From-Via-To
			whereElement = static_cast <DOMElement*> (routeWhereResult->getNodeValue());
			along = findNode(domDoc, whereElement, L"./Along", alongResult);
			towards = findNode(domDoc, whereElement, L"./Towards", towardsResult);
			fromViaTo = findNode(domDoc, whereElement, L"./From-Via-To", fromViaToResult);
			/*
			from RouteWhereType:
			<xs:element name="Along" type="LocationType"/>

			<xs:complexType name="LocationType">
			<xs:choice>
			<xs:element name="LocationID" type="LabelType"/>
			<xs:element name="Coords" type="Coordinates"/>
			</xs:choice>
			</xs:complexType>

			<xs:complexType name="Coordinates">
			<xs:choice>
			<xs:element ref="GDC"/>
			</xs:choice>
			</xs:complexType>
			<xs:element name="GDC">
			<xs:complexType>
			<xs:all>
			<xs:element ref="Latitude"/>
			<xs:element ref="Longitude"/>
			<xs:element ref="ElevationAGL" minOccurs="0"/>
			</xs:all>
			</xs:complexType>
			</xs:element>
			<xs:element name="ElevationAGL">
			<xs:simpleType>
			<xs:restriction base="xs:float">
			<xs:minInclusive value="-1000.0"/>
			<xs:maxInclusive value="100000.0"/>
			</xs:restriction>
			</xs:simpleType>
			</xs:element>
			*/
			// parse Along
			if (along)
			{
				//if((testError = printNoCBML(submitter, thisObject, "RouteWhere/Along")))
				//	return testError; //TODO MXGR: I can't see the point of doing this check
				targetControlMeasure->c_RouteWhereRouteWhereType = L"Along";
				Point* alongTowardsFromLocationPoint =
					targetControlMeasure->c_RouteWhereAlongTowardsFromLocation;

				// look for either LocationID or GDC point
				DOMElement* alongElement = static_cast <DOMElement*> (alongResult->getNodeValue());
				if (!findAndAssign(domDoc, alongElement, L"./LocationID",
					&(alongTowardsFromLocationPoint->c_PointID)))
					if (!findNode(domDoc, alongElement, L"./Coords/GDC", alongResult))
						return printError(submitter, thisObject, "missing Along LocationID or Coords/GDC");
					else
					{
						alongElement = static_cast <DOMElement*> (alongResult->getNodeValue());
						if ((testError = parseIBMLPoint(
							submitter,
							thisObject,
							domDoc,
							alongElement,
							alongTowardsFromLocationPoint,
							"Along/LocationID")))
							return testError;
					}
			}
			/*
			from RouteWhereType:
			<xs:element name="Towards">
			<xs:complexType>
			<xs:choice>
			<xs:element name="Location" type="LocationType"/>
			<xs:element name="Direction" type="mip:DirectionCodeXmlType"/>
			</xs:choice>
			</xs:complexType>
			</xs:element>
			*/
			// parse Towards
			else if (towards)
			{
				/*if((testError = printNoCBML(submitter, thisObject, "RouteWhere/Towards")))
				return testError;*/ //TODO MXGR: I can't see the point of doing this check

				// Towards Location or Direction
				int towardsLocation = findNode(domDoc, whereElement, L"./Towards/Location", towardsResult);
				int towardsDirection = findNode(domDoc, whereElement, L"./Towards/Direction", towardsResult);
				if (towardsLocation + towardsDirection == 0)
					return printError(submitter, thisObject, "missing Towards/Location or Towards/Direction");
				targetControlMeasure->c_RouteWhereRouteWhereType = L"Towards";

				// Towards Location
				if (towardsLocation)
				{

					Point* towardsLocationPoint =
						targetControlMeasure->c_RouteWhereAlongTowardsFromLocation;
					if (!findAndAssign(
						domDoc,
						whereElement,
						L"./Towards/Location/LocationID",
						&(towardsLocationPoint->c_PointID)))
					{
						if (!findNode(
							domDoc,
							whereElement,
							L"./Towards/Location/Coords/GDC",
							towardsResult))
							return printError(submitter, thisObject, "missing Along LocationID or Coords/GDC");
						else
						{
							DOMElement* towardsElement = static_cast <DOMElement*> (towardsResult->getNodeValue());
							if ((testError = parseIBMLPoint(
								submitter,
								thisObject,
								domDoc,
								towardsElement,
								towardsLocationPoint,
								"Towards/Location/GDC")))
								return testError;
						}
					}
				}
				else
				{
					// Towards Direction
					if (!findAndAssign(domDoc, whereElement, L"./Towards/Direction",
						&(targetControlMeasure->c_RouteWhereDirectionCode)))
						return printError(submitter, thisObject, "missing Towards/Direction");
				}
				//return NULL;  removed, need to delete before returning

			}// end Towards

			 /*
			 from RouteWhereType:
			 <xs:element name="From-Via-To">
			 <xs:complexType>
			 <xs:all>
			 <xs:element name="From" type="LocationType" minOccurs="0"/>
			 <xs:element name="Via" minOccurs="0">
			 <xs:complexType>
			 <xs:sequence maxOccurs="unbounded">
			 <xs:element name="Waypoint">
			 <xs:complexType>
			 <xs:sequence>
			 <xs:element name="Location" type="LocationType"/>
			 </xs:sequence>
			 <xs:attribute name="sequence" type="xs:int" use="required"/>
			 </xs:complexType>
			 </xs:element>
			 </xs:sequence>
			 </xs:complexType>
			 </xs:element>
			 <xs:element name="To" type="LocationType"/>
			 </xs:all>
			 </xs:complexType>
			 </xs:element>
			 */
			 // parse From-Via-To
			else if (fromViaTo)
			{
				DOMElement* fromViaToElement;

				// parse From
				targetControlMeasure->c_RouteWhereRouteWhereType = L"From-Via-To";
				Point* fromViaToPoint =
					targetControlMeasure->c_RouteWhereAlongTowardsFromLocation;

				// choice of LocationID or Coords/GDC
				if (!findAndAssign(
					domDoc,
					whereElement,
					L"./From-Via-To/From/LocationID",
					&(fromViaToPoint->c_PointID)))
				{
					if (!findNode(
						domDoc,
						whereElement,
						L"./From-Via-To/From/Coords/GDC",
						fromViaToResult))
						return printError(submitter, thisObject, "missing From-Via-To/From LocationID or Coords/GDC");
					else
					{
						fromViaToElement = static_cast <DOMElement*> (fromViaToResult->getNodeValue());
						if ((testError = parseIBMLPoint(
							submitter,
							thisObject,
							domDoc,
							fromViaToElement,
							fromViaToPoint,
							"From-Via-To/From")))
							return testError;
					}
				}

				// count Via Locations
				numVias = findNode(domDoc, whereElement, L"./From-Via-To/Via/Waypoint", fromViaToResult);

				// parse 0 or more Via
				if (numVias > 0)
				{
					for (viaNum = 0; viaNum < numVias; ++viaNum)
					{
						// allocate a new point and push into vector
						Point *viaLocationPoint = new Point;
						targetControlMeasure->c_RouteWhereViaLocations.push_back(viaLocationPoint);

						// get the attribute Sequence from the element
						fromViaToResult->snapshotItem(viaNum);
						fromViaToElement = static_cast <DOMElement*> (fromViaToResult->getNodeValue());
						const XMLCh* sequenceAttributeValue = fromViaToElement->getAttribute(L"Sequence");
						if (sequenceAttributeValue == NULL)
							return printError(submitter, thisObject, "missing Sequence attribute on From-Via-To/Via/Waypoint");
						viaLocationPoint->c_Sequence = wstring(sequenceAttributeValue);

						// choice of LocationID or Coords/GDC
						if (!findAndAssign(
							domDoc,
							fromViaToElement,
							L"./LocationID",
							&(viaLocationPoint->c_PointID)))
						{
							if (!findNode(
								domDoc,
								fromViaToElement,
								L"./Coords/GDC",
								pointResult))
								return printError(submitter, thisObject, "missing Via/Waypoint LocationID or Coords/GDC");
							else
							{
								// pull parameters out of the Location
								DOMElement* pointElement = static_cast <DOMElement*> (pointResult->getNodeValue());
								if ((testError = parseIBMLPoint(
									submitter,
									thisObject,
									domDoc,
									pointElement,
									viaLocationPoint,
									"Via/Waypoint/Coords")))
									return testError;
							}
						}
					}
				}// end if(numVia > 0)

				 // parse To
				Point* toLocationPoint = targetControlMeasure->c_RouteWhereToLocation;
				if (!findAndAssign(
					domDoc,
					whereElement,
					L"./From-Via-To/To/LocationID",
					&(toLocationPoint->c_PointID)))
				{
					// choice of LocationID or Coords/GDC
					if (!findNode(
						domDoc,
						whereElement,
						L"./From-Via-To/To/Coords/GDC",
						pointResult))
						return printError(submitter, thisObject, "missing From-Via-To/To LocationID or Coords/GDC");
					else
					{
						DOMElement* toElement = static_cast <DOMElement*> (pointResult->getNodeValue());
						if ((testError = parseIBMLPoint(
							submitter,
							thisObject,
							domDoc,
							toElement,
							toLocationPoint,
							"From-Via-To/To")))
							return testError;
					}
				}
			}// end else if(FromViaTo)	
			else printError(submitter, thisObject, "missing Along, Towards, or From-Via-To");

			//return NULL; removed, need to delete before returning

		} // end parse RouteWhere
		else //parse AtWhere
		{
			/*
			<xs:complexType name="AtWhereType">
			<xs:choice>
			<xs:element name="JBMLAtWhere" type="JBMLAtWhereType"/>
			</xs:choice>
			</xs:complexType>

			<xs:complexType name="JBMLAtWhereType">
			<xs:sequence>
			<xs:element name="WhereLabel" type="LabelType"/>
			<xs:element name="WhereCategory" type="WhereCategoryType"/>
			<xs:element name="jc3iedmWhereCategory" type="mip:ControlFeatureTypeCategoryCodeXmlType" minOccurs="0"/>
			<xs:element name="WhereCategoryEchelon" type="mip:UnitTypeSizeCodeXmlType" minOccurs="0"/>
			<xs:element name="WhereClass" type="WhereClassType"/>
			<xs:element name="WhereValue" type="WhereValueType"/>
			<xs:element name="WhereQualifier" type="WhereQualifierType"/>
			<xs:element name="VelocityVector" type="VelocityVectorType" minOccurs="0"/>
			</xs:sequence>
			</xs:complexType>
			*/
			/*
			from JBMLAtWhereType:
			<xs:element name="WhereLabel" type="LabelType"/>
			*/
			// parse WhereLabel
			DOMElement* atWhereElement = static_cast <DOMElement*> (atWhereResult->getNodeValue());
			if (!findAndAssign(domDoc, atWhereElement, L"./WhereLabel",
				&(targetControlMeasure->c_AtWhereWhereLabel)))
				return printError(submitter, thisObject, "missing WhereLabel");
			/*
			from JBMLAtWhereType:
			<xs:element name="WhereCategory" type="WhereCategoryType"/>

			<xs:simpleType name="WhereCategoryType">
			<xs:restriction base="xs:string">
			<xs:enumeration value="AIRCONTROLPOINT"/>
			<xs:enumeration value="CHECKPOINT"/>
			<xs:enumeration value="CONTROLPOINT"/>
			<xs:enumeration value="DECISIONPOINT"/>
			<xs:enumeration value="DROPPOINT"/>
			<xs:enumeration value="PASSAGEPOINT"/>
			<xs:enumeration value="RALLYPOINT"/>
			<xs:enumeration value="RENDEZVOUSPOINT"/>
			<xs:enumeration value="ALTERNATESUPPLYROUTE"/>
			<xs:enumeration value="COORDINATIONFIRELINE"/>
			<xs:enumeration value="BOUNDARYLINE"/>
			<xs:enumeration value="FIRESUPPORTCOORDINATIONLINE"/>
			<xs:enumeration value="FORWARDLINEOFTROOPS"/>
			<xs:enumeration value="LINEOFADVANCE"/>
			<xs:enumeration value="LINEOFDEPARTURE"/>
			<xs:enumeration value="LINEOFCONTACT"/>
			<xs:enumeration value="OBSTACLELINE"/>
			<xs:enumeration value="PHASELINE"/>
			<xs:enumeration value="ROUTE"/>
			<xs:enumeration value="MAINSUPPLYROUTE"/>
			<xs:enumeration value="AREAOFINTEREST"/>
			<xs:enumeration value="AREAOFOPERATION"/>
			<xs:enumeration value="ASSEMBLYAREA"/>
			<xs:enumeration value="ARTILLERYAREA"/>
			<xs:enumeration value="ASSAULTPOSITION"/>
			<xs:enumeration value="ATTACKBYFIREPOSITION"/>
			<xs:enumeration value="BATTLEPOSITION"/>
			<xs:enumeration value="CONTROLAREA"/>
			<xs:enumeration value="ENGAGEMENTAREA"/>
			<xs:enumeration value="MAINBATTLEAREA"/>
			<xs:enumeration value="MINEDAREA"/>
			<xs:enumeration value="NAMEDAREAOFINTEREST"/>
			<xs:enumeration value="OBJECTIVEAREA"/>
			<xs:enumeration value="RECONNAISSANCEAREA"/>
			<xs:enumeration value="SLOWGOAREA"/>
			<xs:enumeration value="SUPPLYAREA"/>
			<xs:enumeration value="NOGOAREA"/>
			<xs:enumeration value="WAYPOINT"/>
			<xs:enumeration value="INITIALPOINT"/>
			<xs:enumeration value="NOGOAREA"/>
			<xs:enumeration value="DEPARTUREBASE"/>
			<xs:enumeration value="RECOVERYBASE"/>
			<xs:enumeration value="TARGET"/>
			<xs:enumeration value="BENUMBER"/>
			<xs:enumeration value="GENCOORDINATE"/>
			</xs:restriction>
			</xs:simpleType>
			*/
			// parse WhereCategory
			if (!findAndAssign(domDoc, atWhereElement, L"./WhereCategory",
				&(targetControlMeasure->c_AtWhereWhereCategory)))
				return printError(submitter, thisObject, "missing WhereCategory");
			/*
			from JBMLAtWhereType:
			<xs:element name="jc3iedmWhereCategory" type="mip:ControlFeatureTypeCategoryCodeXmlType" minOccurs="0"/>
			*/
			// parse jc3iedmWhereCategory (optional)
			findAndAssign(domDoc, atWhereElement, L"./jc3iedmWhereCategory",
				&(targetControlMeasure->c_AtWherejc3iedmWhereCategory));
			/*
			from JBMLAtWhereType:
			<xs:element name="WhereCategoryEchelon" type="mip:UnitTypeSizeCodeXmlType" minOccurs="0"/>
			*/
			// parse WhereCategoryEchelon (optional)
			findAndAssign(domDoc, atWhereElement, L"./WhereCategoryEchelon",
				&(targetControlMeasure->c_AtWhereWhereCategoryEchelon));
			/*
			from JBMLAtWhereType:
			<xs:element name="WhereClass" type="WhereClassType"/>

			<xs:simpleType name="WhereClassType">
			<xs:restriction base="xs:string">
			<xs:enumeration value="PT"/>
			<xs:enumeration value="LN"/>
			<!-- Needed but Not Implemented -->
			<xs:enumeration value="SURFAC"/>
			</xs:restriction>
			</xs:simpleType>
			*/
			// parse WhereClass
			if (!findAndAssign(domDoc, atWhereElement, L"./WhereClass",
				&(targetControlMeasure->c_AtWhereWhereClass)))
				return printError(submitter, thisObject, "missing WhereClass");
			/*
			from JBMLAtWhereType:
			<xs:element name="WhereValue" type="WhereValueType"/>

			<xs:complexType name="WhereValueType">
			<xs:choice>
			<xs:element name="WhereLocation" type="WhereLocationType" maxOccurs="unbounded"/>
			</xs:choice>
			</xs:complexType>

			<xs:complexType name="WhereLocationType">
			<xs:complexContent>
			<xs:extension base="Coordinates">
			<xs:attribute name="Sequence" type="xs:integer"/>
			</xs:extension>
			</xs:complexContent>
			</xs:complexType>
			*/
			// parse WhereValue Points
			numPoints = findNode(domDoc, atWhereElement, L"./WhereValue/WhereLocation", sequenceResult);
			for (pointNum = 0; pointNum < numPoints; pointNum++)
			{
				// allocate new Point struct and push it on Vector in Task struct
				Point *point = new(Point);
				targetControlMeasure->c_AtWherePoints.push_back(point);

				// get the attribute Sequence from the WhereLocation element
				sequenceResult->snapshotItem(pointNum);
				DOMElement* sequenceElement = static_cast <DOMElement*> (sequenceResult->getNodeValue());
				const XMLCh* sequenceAttributeValue = sequenceElement->getAttribute(L"Sequence");
				if (sequenceAttributeValue == NULL)
					return printError(submitter, thisObject, "missing Sequence attribute on WhereValue/WhereLocation");
				std::wstring sequenceAttributeString(sequenceAttributeValue);
				if (sequenceAttributeString.empty())// UK air hack
				{
					print(submitter, thisObject, "empty Sequence for WhereLocation set to 0 for ",
						targetControlMeasure->c_AtWhereWhereLabel);
					point->c_Sequence = L"0";
				}
				else
					point->c_Sequence = sequenceAttributeString;

				// parse the coordinate values
				if (!findNode(domDoc, sequenceElement, L"./GDC", pointResult))
					return printError(submitter, thisObject, "missing WhereLocation/GDC in WhereValue");
				DOMElement* pointElement = static_cast <DOMElement*> (pointResult->getNodeValue());
				if ((testError = parseIBMLPoint(
					submitter,
					thisObject,
					domDoc,
					pointElement,
					point,
					"WhereValue/WhereLocation/GDC")))
					return testError;

			}// end Point loop
			 /*
			 from JBMLAtWhereType:
			 <xs:element name="WhereQualifier" type="WhereQualifierType"/>

			 <xs:simpleType name="WhereQualifierType">
			 <xs:restriction base="xs:string">
			 <xs:enumeration value="AT"/>
			 <xs:enumeration value="ALONG"/>
			 <xs:enumeration value="BETWEEN"/>
			 <xs:enumeration value="FROM"/>
			 <xs:enumeration value="IN"/>
			 <xs:enumeration value="ON"/>
			 <xs:enumeration value="TO"/>
			 </xs:restriction>
			 </xs:simpleType>
			 */
			 // parse WhereQualifier
			if (!findAndAssign(domDoc, atWhereElement, L"./WhereQualifier",
				&(targetControlMeasure->c_AtWhereWhereQualifier)))
				return printError(submitter, thisObject, "missing WhereQualifier");
			/*
			from JBMLAtWhereType:
			<xs:element name="VelocityVector" type="VelocityVectorType" minOccurs="0"/>

			<xs:complexType name="VelocityVectorType">
			<xs:sequence>
			<xs:element name="Magnitude" type="xs:float"/>
			<xs:element name="BearingDegrees" type="xs:float"/>
			</xs:sequence>
			</xs:complexType>
			*/
			// parse VelocityVector (optional)
			if (findNode(domDoc, atWhereElement, L"./VelocityVector", velocityVectorResult) == 0)
				return NULL;
			if (!findAndAssign(domDoc, atWhereElement, L"./VelocityVector/Magnitude",
				&(targetControlMeasure->c_AtWhereVelocityMagnitude)))
				return printError(submitter, thisObject, "missing VelocityVector/Magnitude");
			if (!findAndAssign(domDoc, atWhereElement, L"./VelocityVector/BearingDegrees",
				&(targetControlMeasure->c_AtWhereVelocityBearingDegrees)))
				return printError(submitter, thisObject, "missing VelocityVector/BearingDegrees");
		} //end parse AtWhere

		if (routeWhereResult != NULL)
			delete routeWhereResult;
		if (atWhereResult != NULL)
			delete atWhereResult;
		if (routeWhereResult != NULL)
			delete routeWhereResult;
		if (atWhereResult != NULL)
			delete atWhereResult;
		if (alongResult != NULL)
			delete alongResult;
		if (towardsResult != NULL)
			delete towardsResult;
		if (fromViaToResult != NULL)
			delete fromViaToResult;
		if (sequenceResult != NULL)
			delete sequenceResult;
		if (pointResult != NULL)
			delete pointResult;
		if (velocityVectorResult != NULL)
			delete velocityVectorResult;

		return NULL;

}// end parseIBMLWhere()


 // parse an IBML GDC point
char* parseIBMLPoint(
	wstring submitter,
	CWISEDriver* thisObject,
	DOMDocument* domDoc,
	DOMElement*  pointElement,
	Point* point,
	string errorLabel)
{
	// Get the point details: Latitude, Longitude, and optional ElevationAGL
	if (!findAndAssign(domDoc, pointElement, L"./Latitude", &(point->c_Latitude)))
		return printError(submitter, thisObject, "missing Latitude in ", errorLabel);
	if (!findAndAssign(domDoc, pointElement, L"./Longitude", &(point->c_Longitude)))
		return printError(submitter, thisObject, "missing Longitude in ", errorLabel);
	findAndAssign(domDoc, pointElement, L"./ElevationAGL", &(point->c_ElevationAGL));

	return NULL;

}// end parseIBMLPoint()


 // parse an ObservedWho (AffectedWho)
char* parseIBMLObservedWho(
	wstring submitter,
	CWISEDriver* thisObject,
	DOMDocument* domDoc,
	DOMElement* observedWhoElement,
	ObservedWho* observedWho,
	TaskData* task)
{
	DOMXPathResult* result = NULL;

	// fill target structure with either UnitID or a UnitDescription
	if (!findAndAssign(domDoc, observedWhoElement, L"./UnitID", &(observedWho->c_UnitID)))
	{
		if (!findNode(domDoc, observedWhoElement, L"./UnitDescription", result))
			return printError(submitter, thisObject, "AffectedWho missing UnitID or UnitDescription");

		// parse elements of UnitDescription
		if (!findAndAssign(domDoc, observedWhoElement, L"./UnitDescription/Hostility", &(observedWho->c_Hostility)))
			return printError(submitter, thisObject, "GeneralStatusReport missing Executer/UnitDescription/Hostility");
		if (!findAndAssign(domDoc, observedWhoElement, L"./UnitDescription/Size", &(observedWho->c_Size)))
			return printError(submitter, thisObject, "GeneralStatusReport missing Executer/UnitDescription/Size");
		if (!findAndAssign(domDoc, observedWhoElement, L"./UnitDescription/ArmCategory", &(observedWho->c_ArmCategory)))
			return printError(submitter, thisObject, "GeneralStatusReport missing Executer/UnitDescription/ArmCategory");
		findAndAssign(domDoc, observedWhoElement, L"./UnitDescription/Qualifier", &(observedWho->c_Qualifier));
	}

	if (result != NULL)
		delete result;
	return NULL;

}// end parseIBMLObservedWho()

 // parse one level of TaskOrg tree
int parseIBMLTaskOrg(
	wstring submitter,
	CWISEDriver* thisObject,
	DOMDocument* domDoc,
	OrderData *order,
	TaskOrg *taskOrg,
	DOMElement* parentTaskOrgElement,
	wstring parentUnitID)
{
	// parse the UnitID which must be here
	DOMXPathResult* childTaskOrgResult = NULL;
	DOMElement* childTaskOrgElement;

	// save this TaskOrg UnitID and ParentUnitID
	findAndAssign(domDoc, parentTaskOrgElement, L"./UnitID", &(taskOrg->c_UnitID));
	wstring thisUnitID = taskOrg->c_UnitID;
	if (thisUnitID == parentUnitID)
	{
		print(submitter, thisObject, "error - TaskOrg OID cannot be its own parent:", thisUnitID);
		return -1;
	}
	taskOrg->c_ParentUnitID = parentUnitID;

	// find number of child TaskOrgs
	int numChildren = findNode(domDoc, parentTaskOrgElement, L"./TaskOrganization", childTaskOrgResult);

	// parse each child once to get its OID
	int childNum;
	for (childNum = 0; childNum < numChildren; ++childNum)
	{
		// parse each child unit adding its lower layers 
		childTaskOrgResult->snapshotItem(childNum);
		childTaskOrgElement = static_cast <DOMElement*> (childTaskOrgResult->getNodeValue());
		ChildUnit *childUnit = new(struct ChildUnit);
		if (!findAndAssign(domDoc, childTaskOrgElement, L"./UnitID", &(childUnit->c_UnitID)))
		{
			print(submitter, thisObject, "error - missing UnitID in TaskOrg under parent:", parentUnitID);
			return -1;
		}
		if (childUnit->c_UnitID == thisUnitID)
		{
			print(submitter, thisObject, "error - TaskOrg UnitID cannot be its own parent:", thisUnitID);
			return -1;
		}
		taskOrg->c_ChildTaskOrg.push_back(childUnit);
	}

	// parse all levels down by calling ourself
	for (int childNum = 0; childNum < numChildren; ++childNum)
	{
		// parse each child unit adding its lower layers 
		childTaskOrgResult->snapshotItem(childNum);
		childTaskOrgElement = static_cast <DOMElement*> (childTaskOrgResult->getNodeValue());
		struct TaskOrg *childTaskOrg = new(struct TaskOrg);
		order->c_TaskOrgVector.push_back(childTaskOrg);

		// call ourselves to do this for the next level
		if (parseIBMLTaskOrg(
			submitter,
			thisObject,
			domDoc,
			order,
			childTaskOrg,
			childTaskOrgElement,
			taskOrg->c_UnitID) < 0)
			return -1;
	}
	if (childTaskOrgResult != NULL)
		delete childTaskOrgResult;

	return numChildren;

}// end parseIBMLTaskOrg()


 /****************************
 *	generateIBMLReportQuery	*
 ****************************/
WISE_HANDLE fetchDBH, fetchObject, fetchAttribute;
IWISEDriverSink *fetchSink;


char* generateIBMLReportQuery(
	char* fetchError,
	wstring w_submitter,
	CWISEDriver* thisObject,
	IWISEDriverSink* sink,
	WISE_HANDLE hDBHandle,
	WISE_HANDLE hObject,
	string reportType)
{
	string bml;
	WISE_HANDLE hAttribute;

	// data that may need tweaked
	string reporterWho = fetchWSS(fetchError, w_submitter, thisObject, sink, hDBHandle, hObject, "ReporterWho");
	if (reporterWho == "")reporterWho = fetchWSS(fetchError, w_submitter, thisObject, sink, hDBHandle, hObject, "ReportingDataOID");
	string reportID = fetchWSS(fetchError, w_submitter, thisObject, sink, hDBHandle, hObject, "ReportID");
	string insertReportType = reportType;

	// MSG-085 hack: translate PSR to GSR for IBML and FKIE
	if (reportType == "PositionStatusReport")
		insertReportType = "GeneralStatusReport";

	// XML Header 
	bml = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>";

	// Opening tag and namespace declaration for IBML
	bml += "<ibml:BMLReport xmlns:ibml=\"http://netlab.gmu.edu/IBML\"><ibml:Report>";

	// Common report elements (we only support StatusReports in IBML)
	bml +=
		"<ibml:CategoryOfReport>StatusReport</ibml:CategoryOfReport><ibml:TypeOfReport>" +
		insertReportType +
		"</ibml:TypeOfReport><ibml:StatusReport><ibml:" +
		insertReportType +
		">";

	if (reportType != "ObserveTaskReport")
	{
		bml +=
			"<ibml:ReporterWho><ibml:UnitID>" +
			reporterWho +
			"</ibml:UnitID></ibml:ReporterWho>";
	}
	/*
	<xs:complexType name="SpotReportType">
	<xs:sequence>
	<xs:element name="Hostility" type="jc3iedm:ObjectItemHostilityStatusCode" minOccurs="0"/>
	<xs:element name="Executer" type="jc3iedm:OIDType" minOccurs="0"/>
	<xs:element name="AtWhere" type="cbml:PointLightType" minOccurs="0"/>
	<xs:element name="Context" type="jc3iedm:TextTypeVar80" minOccurs="0"/>
	<xs:element name="Parameters">
	<xs:complexType>
	<xs:sequence>
	<xs:element name="Size" type="xs:string" minOccurs="0"/>
	<xs:element name="Activity" type="xs:string" minOccurs="0"/>
	<xs:element name="Location" type="cbml:PointLightType" minOccurs="0"/>
	<xs:element name="Unit" type="jc3iedm:OIDType" minOccurs="0"/>
	<xs:element name="Equipment" type="jc3iedm:VehicleTypeCategoryCode"
	minOccurs="0"/>
	<xs:element name="SendersAssesment" type="xs:string" minOccurs="0"/>
	<xs:element name="Narrative" type="xs:string" minOccurs="0"/>
	<xs:element name="Authentication" type="xs:string" minOccurs="0"/>
	</xs:sequence>
	</xs:complexType>
	</xs:element>
	</xs:sequence>
	</xs:complexType>
	*/
	if (reportType == "ObserveTaskReport")
	{
		// Hostility
		string sr =
			"<ibml:Hostility>" +
			fetchWSS(fetchError, w_submitter, thisObject, sink, hDBHandle, hObject, "HostilityCode") +
			"</ibml:Hostility>";

		// Executer, an ObservedWho
		string executer = fetchWSS(fetchError, w_submitter, thisObject, sink, hDBHandle, hObject, "UnitName");//debugx ExecuterUnitID
		sr += "<ibml:Executer><ibml:UnitID>" + executer + "</ibml:UnitID></ibml:Executer>";

		// AtWhere: Latitude, Longitude, and optional Elevation
		string latitude = fetchWSS(fetchError, w_submitter, thisObject, sink, hDBHandle, hObject, "Latitude");
		if (latitude != "")
		{
			sr +=
				"<ibml:AtWhere><bml:GDC><bml:Latitude>" +
				fetchWSS(fetchError, w_submitter, thisObject, sink, hDBHandle, hObject, "SpotLatitude") +
				"</bml:Latitude><bml:Longitude>" +
				fetchWSS(fetchError, w_submitter, thisObject, sink, hDBHandle, hObject, "SpotLongitude") +
				"</bml:Longitude>";
			string elevation = fetchWSS(fetchError, w_submitter, thisObject, sink, hDBHandle, hObject, "SpotElevation");
			if (elevation != "")
				sr +=
				"<bml:ElevationAGL>" +
				elevation +
				"</bml:ElevationAGL>";

			sr +=
				"</ibml:GDC></ibml:AtWhere>";
		}

		// Context (optional)
		string context = ofetchWSS(sink, hDBHandle, hObject, "Context");
		if (context != "")
			sr +=
			"<ibml:Context>" +
			context +
			"</ibml:Context>";

		// Parameters (all optional)

		// Size
		string size = ofetchWSS(sink, hDBHandle, hObject, "ExecuterSize");
		if (size != "")
			sr += "<ibml:Size>" + size + "</ibml:Size>";

		// Activity
		string activity = ofetchWSS(sink, hDBHandle, hObject, "ExecuterActivity");
		if (activity != "")
			sr += "<ibml:Activity>" + activity + "</ibml:Activity>";

		// Location
		latitude = ofetchWSS(sink, hDBHandle, hObject, "SpotLatitude");
		if (latitude != "")
		{
			sr += "<ibml:Location><ibml:GDC>";
			string gdcoid = ofetchWSS(sink, hDBHandle, hObject, "LocationID");
			if (gdcoid != "")
				sr +=
				"<ibml:OID>" +
				gdcoid +
				"</ibml:OID>";
			sr +=
				"<ibml:LocationLatitude>" +
				latitude +
				"</ibml:Latitude><ibml:Longitude>" +
				fetchWSS(fetchError, w_submitter, thisObject, sink, hDBHandle, hObject, "LocationLongitude") +
				"</ibml:Longitude>";
			string elevation = ofetchWSS(sink, hDBHandle, hObject, "LocationElevation");
			if (elevation != "")
				sr +=
				"<ibml:Elevation>" +
				elevation +
				"</ibml:Elevation>";
			sr += "</ibml:GDC></ibml:Location>";
		}// end if latitude 

		 // Unit
		string unit = ofetchWSS(sink, hDBHandle, hObject, "UnitName");//debugx ExecuterParametersUnitID
		if (unit != "")
			sr += "<ibml:Unit>" + unit + "</ibml:Unit>";

		// Equipment
		string equipment = ofetchWSS(sink, hDBHandle, hObject, "ExecuterEquipment");
		if (equipment != "")
			sr += "<ibml:Equipment>" + equipment + "</ibml:Equipment>";

		// SendersAssesment
		string sendersAssessment = ofetchWSS(sink, hDBHandle, hObject, "SendersAssesment");
		if (equipment != "")
			sr += "<ibml:SendersAssesment>" + sendersAssessment + "</ibml:SendersAssesment>";

		// Narrative
		string narrative = ofetchWSS(sink, hDBHandle, hObject, "Narrative");
		if (narrative != "")
			sr += "<ibml:Narrative>" + narrative + "</ibml:Narrative>";

		// Authentication
		string authentication = ofetchWSS(sink, hDBHandle, hObject, "Authentication");
		if (authentication != "")
			sr += "<ibml:Authentication>" + authentication + "</ibml:Authentication>";

		bml += sr;
	}
	/*
	<xs:element name="GeneralStatusReport">
	<xs:complexType>
	<xs:sequence>
	<xs:element name="ReporterWho" type="bml:WhoType"/>
	<xs:element name="Context" type="bml:LabelType" minOccurs="0"/>
	<xs:element name="Hostility" type="mip:ObjectItemHostilityStatusCodeXmlType"/>
	<xs:element name="Executer" type="bml:ObservedWhoType"/>
	<xs:element name="OpStatus" type="mip:OrganisationStatusOperationalStatusCodeXmlType"/>
	<xs:element name="WhereLocation" type="bml:WhereLocationType"/>
	<xs:element name="VelocityVector" type="bml:VelocityVectorType" minOccurs="0"/>
	<xs:element name="When" type="mip:Datetime18XmlType"/>
	<xs:element name="ReportID" type="bml:LabelType"/>
	<xs:element name="Credibility" type="bml:CredibilityType"/>
	</xs:sequence>
	</xs:complexType>
	</xs:element>

	<xs:complexType name="ObservedWhoType">
	<xs:choice>
	<xs:element name="UnitID" type="LabelType"/>
	<xs:element name="UnitDescription">
	<xs:complexType>
	<xs:sequence>
	<xs:element name="Hostility" type="mip:ObjectItemHostilityStatusCodeXmlType"/>
	<xs:element name="Size" type="mip:UnitTypeSizeCodeXmlType"/>
	<xs:element name="ArmCategory" type="mip:UnitTypeArmCategoryCodeXmlType"/>
	<xs:element name="Qualifier" type="mip:UnitTypeQualifierCodeXmlType" minOccurs="0"/>
	</xs:sequence>
	</xs:complexType>
	</xs:element>
	</xs:choice>
	</xs:complexType>

	<xs:complexType name="CredibilityType">>
	<xs:all>
	<xs:element name="Source" type="mip:ReportingDataSourceTypeCodeXmlType"/>
	<xs:element name="Reliability" type="mip:ReportingDataReliabilityCodeXmlType"/>
	<xs:element name="Certainty" type="mip:ReportingDataCredibilityCodeXmlType"/>
	</xs:all>>
	</xs:complexType>
	*/
	else if (reportType == "GeneralStatusReport")
	{
		string gsr = "";

		// Context (optional)
		string context = ofetchWSS(sink, hDBHandle, hObject, "Context");
		gsr +=
			"<ibml:Context>" +
			context +
			"</ibml:Context>";

		// Hostility
		gsr +=
			"<ibml:Hostility>" +
			fetchWSS(fetchError, w_submitter, thisObject, sink, hDBHandle, hObject, "HostilityCode") +
			"</ibml:Hostility>";

		// Executer, an ObservedWho
		// NOTES: (1) for observations that do not see UnitName we could have no ExecuterUnitID
		//            we can determine this case because there will be an ExecuterHostility
		//        (2) key for ReportDetail is UnitName which comes from ExecuterUnitID
		string executerHostility =
			fetchWSS(fetchError, w_submitter, thisObject, sink, hDBHandle, hObject, "ExecuterHostility");
		if (executerHostility == "")
			gsr += "<ibml:Executer><bml:Taskee><bml:UnitID>" +
			fetchWSS(fetchError, w_submitter, thisObject, sink, hDBHandle, hObject, "UnitName") +
			"</bml:UnitID></bml:Taskee></ibml:Executer>";
		else
		{
			gsr +=
				"<ibml:Executer><bml:Taskee><ibml:Hostility>" +
				executerHostility +
				"</ibml:Hostility><ibml:Size>" +
				fetchWSS(fetchError, w_submitter, thisObject, sink, hDBHandle, hObject, "ExecuterSize") +
				"</ibml:Size><ibml:ArmCategory>" +
				fetchWSS(fetchError, w_submitter, thisObject, sink, hDBHandle, hObject, "ExecuterArmCategory") +
				"</ibml:ArmCategory>";
			string executerQualifier = ofetchWSS(sink, hDBHandle, hObject, "ExecuterQualifier");
			if (executerQualifier != "")
				gsr +=
				"<ibml:Qualifier>" +
				executerQualifier +
				"</ibml:Qualifier>";
			gsr += "</bml:Taskee></ibml:Executer>";
		}

		// OpStatus
		gsr +=
			"<ibml:OpStatus>" +
			fetchWSS(fetchError, w_submitter, thisObject, sink, hDBHandle, hObject, "OperationalStatus") +
			"</ibml:OpStatus>";

		// Where: Latitude, Longitude, and optional Elevation
		gsr += generateIBMLReportQueryGeoCoords(fetchError, w_submitter, thisObject, sink, hDBHandle, hObject);

		// VelocityVector (optional)
		string magnitude = ofetchWSS(sink, hDBHandle, hObject, "Velocity_Magnitude");
		if (magnitude != "")
			gsr +=
			"<ibml:VelocityVector><ibml:Magnitude>" +
			magnitude +
			"</ibml:Magnitude><ibml:BearingDegrees>" +
			fetchWSS(fetchError, w_submitter, thisObject, sink, hDBHandle, hObject, "Velocity_BearingDegrees>") +
			"</ibml:BearingDegrees></ibml:VelocityVector>";

		// When & ReportID
		gsr +=
			"<ibml:When>" +
			fetchWSS(fetchError, w_submitter, thisObject, sink, hDBHandle, hObject, "Date") +
			"</ibml:When><ibml:ReportID>" +
			reportID +
			"</ibml:ReportID>";

		// Credibility
		gsr +=
			"<ibml:Credibility><ibml:Source>" +
			fetchWSS(fetchError, w_submitter, thisObject, sink, hDBHandle, hObject, "CredibilitySource") +
			"</ibml:Source><ibml:Reliability>" +
			fetchWSS(fetchError, w_submitter, thisObject, sink, hDBHandle, hObject, "CredibilityReliability") +
			"</ibml:Reliability><ibml:Certainty>" +
			fetchWSS(fetchError, w_submitter, thisObject, sink, hDBHandle, hObject, "CredibilityCertainty") +
			"</ibml:Certainty></ibml:Credibility>";

		bml += gsr;
	}
	/*
	<xs:element name="PositionStatusReport">
	<xs:complexType>
	<xs:sequence>
	<xs:element name="Hostility" type="jc3iedm:ObjectItemHostilityStatusCode"/>
	<xs:element name="Executer" type="jc3iedm:OIDType"/>
	<xs:element name="AtWhere" type="cbml:PointLightType"/>
	<xs:element name="Context" type="jc3iedm:TextTypeVar80" minOccurs="0"/>
	</xs:sequence>
	</xs:complexType>
	</xs:element>
	*/
	else if (reportType == "PositionStatusReport")
	{
		/* MSG-085 is translating PSR to GSR for IBML/FKIE, using "unkown" for OpStatus and Credibility

		string psr;
		// Context (optional)
		string context = ofetchWSS(sink, hDBHandle, hObject, "Context");
		psr +=
		"<ibml:Context>" +
		context +
		"</ibml:Context>";

		// Hostility
		psr +=
		"<ibml:Hostility>" +
		fetchWSS(fetchError, w_submitter, thisObject, sink, hDBHandle, hObject, "HostilityCode") +
		"</ibml:Hostility>";

		// Executer, an ObservedWho
		string executer = fetchWSS(fetchError, w_submitter, thisObject, sink, hDBHandle, hObject, "ExecuterUnitID");
		if(executer != "")
		psr += "<ibml:Executer><bml:Taskee><bml:UnitID>" + executer + "</bml:UnitID></bml:Taskee></ibml:Executer>";

		// Where: Latitude, Longitude, and optional Elevation
		psr +=
		"<ibml:WhereLocation><bml:GDC><bml:Latitude>" +
		fetchWSS(fetchError, w_submitter, thisObject, sink, hDBHandle, hObject, "Latitude") +
		"</bml:Latitude><bml:Longitude>" +
		fetchWSS(fetchError, w_submitter, thisObject, sink, hDBHandle, hObject, "Longitude") +
		"</bml:Longitude>";
		string elevation = fetchWSS(fetchError, w_submitter, thisObject, sink, hDBHandle, hObject, "Elevation");
		if(elevation != "")
		psr += "<bml:ElevationAGL>" + elevation + "</bml:ElevationAGL>";

		psr +=
		"</bml:GDC></ibml:WhereLocation>";

		bml += psr;

		*/
		// Context is optional

		// Hostility
		string gsr =
			"<ibml:Hostility>" +
			fetchWSS(fetchError, w_submitter, thisObject, sink, hDBHandle, hObject, "HostilityCode") +
			"</ibml:Hostility>";

		// Executer UnitID only
		gsr +=
			"<ibml:Executer><ibml:UnitID>" +
			fetchWSS(fetchError, w_submitter, thisObject, sink, hDBHandle, hObject, "ExecuterUnitID") +
			"</ibml:UnitID></ibml:Executer>";

		// OpStatus
		gsr +=
			"<ibml:OpStatus>NKN</ibml:OpStatus>";

		// Where
		gsr +=
			"<ibml:WhereLocation><bml:GDC><bml:Latitude>" +
			fetchWSS(fetchError, w_submitter, thisObject, sink, hDBHandle, hObject, "Latitude") +
			"</bml:Latitude><bml:Longitude>" +
			fetchWSS(fetchError, w_submitter, thisObject, sink, hDBHandle, hObject, "Longitude") +
			"</bml:Longitude>";
		string elevation = ofetchWSS(sink, hDBHandle, hObject, "Elevation");
		if (elevation != "")
			gsr += "<bml:ElevationAGL>" + elevation + "</bml:ElevationAGL>";
		gsr +=
			"</bml:GDC></ibml:WhereLocation>";

		// VelocityVector  is optional

		// When & ReportID
		gsr +=
			"<ibml:When>" +
			fetchWSS(fetchError, w_submitter, thisObject, sink, hDBHandle, hObject, "Date") +
			"</ibml:When><ibml:ReportID>" +
			reportID +
			"</ibml:ReportID>";

		// Credibility
		gsr +=
			"<ibml:Credibility><ibml:Source>UNSPEC</ibml:Source><ibml:Reliability>F</ibml:Reliability><ibml:Certainty>IND</fkie:Certainty></fkie:Credibility>";

		bml += gsr;
	}
	/*
	<xs:element name="TaskStatusReport">
	<xs:complexType>
	<xs:sequence>
	<xs:element name="ReportID" type="bml:LabelType"/>
	<xs:element name="When" type="mip:Datetime18XmlType"/>
	<xs:element name="TaskID" type="bml:LabelType"/>
	<xs:element name="Executer" type="bml:ExecuterType"/>
	<xs:element name="TaskStatus" type="mip:ActionTaskStatusProgressCodeXmlType"/>
	</xs:sequence>
	</xs:complexType>
	</xs:element>
	*/
	else if (reportType == "TaskStatusReport")
	{
		string tsr =
			"<ibml:ReportID>" +
			fetchWSS(fetchError, w_submitter, thisObject, sink, hDBHandle, hObject, "ReportID") +
			"</ibml:ReportID><ibml:When>" +
			fetchWSS(fetchError, w_submitter, thisObject, sink, hDBHandle, hObject, "Date") +
			"</ibml:When><ibml:TaskID>" +
			fetchWSS(fetchError, w_submitter, thisObject, sink, hDBHandle, hObject, "TaskID") +
			"</ibml:TaskID><ibml:Executer><ibml:UnitID>" +
			fetchWSS(fetchError, w_submitter, thisObject, sink, hDBHandle, hObject, "ExecuterUnitID") +
			"</ibml:UnitID></ibml:Executer><ibml:TaskStatus>" +
			fetchWSS(fetchError, w_submitter, thisObject, sink, hDBHandle, hObject, "TaskStatus") +
			"</ibml:TaskStatus>";

		bml += tsr;
	}
	/*
	<xs:element name="WhoHoldingStatusReport">
	<xs:complexType>
	<xs:sequence>
	<xs:element name="ReporterWho" type="bml:WhoType"/>
	<xs:element name="When" type="mip:Datetime18XmlType"/>
	<xs:element name="WhoRef" type="bml:WhoType"/>
	<xs:element name="Holding" type="HoldingType"/>
	<xs:element name="Context" type="bml:LabelType" minOccurs="0"/>
	<xs:element name="ReportID" type="bml:LabelType"/>
	</xs:sequence>
	</xs:complexType>
	</xs:element>

	<xs:complexType name="WhoType">
	<xs:choice>
	<xs:element name="UnitID" type="LabelType"/>
	</xs:choice>
	</xs:complexType>

	<xs:complexType name="HoldingType">
	<xs:sequence>
	<xs:element name="NSN_Code" type="xs:string"/>
	<xs:element name="NSN_Name" type="xs:string" minOccurs="0"/>
	<xs:element name="IsEquipment" type="xs:boolean" minOccurs="0"/>
	<xs:element name="OperationalCount" type="xs:double" minOccurs="0"/>
	<xs:element name="TotalQuantity" type="xs:double" minOccurs="0"/>
	<xs:element name="OnHandQuantity" type="xs:double" minOccurs="0"/>
	<xs:element name="RequiredTotalQuantity" type="xs:double" minOccurs="0"/>
	<xs:element name="RequiredOnHandQuantity" type="xs:double" minOccurs="0"/>
	<xs:element name="RequiredCalculationMethodCode" type="xs:double" minOccurs="0"/>
	</xs:sequence>
	</xs:complexType>
	*/
	else if (reportType == "WhoHoldingStatusReport")
	{
		// When, WhoRef, Holding
		string whsr =
			"<ibml:When>" +
			fetchWSS(fetchError, w_submitter, thisObject, sink, hDBHandle, hObject, "Date") +
			"</ibml:When><ibml:WhoRef><ibml:UnitID>" +
			fetchWSS(fetchError, w_submitter, thisObject, sink, hDBHandle, hObject, "WhoRef") +
			"</ibml:UnitID></ibml:WhoRef><ibml:Holding><ibml:NSN_Code>" +
			fetchWSS(fetchError, w_submitter, thisObject, sink, hDBHandle, hObject, "NSN_Code") +
			"</ibml:NSN_Code>";

		// NSN Name (optional)
		string nsnName = ofetchWSS(sink, hDBHandle, hObject, "NSN_Name");
		if (nsnName != "")
			whsr +=
			"<ibml:NSN_Name>" +
			nsnName +
			"</ibml:NSN_Name>";

		// IsEqupment (optional)
		string isEquipment = ofetchWSS(sink, hDBHandle, hObject, "IsEquipment");
		if (isEquipment != "")
			whsr +=
			"<ibml:IsEquipment>" +
			isEquipment +
			"</ibml:IsEquipment>";

		// OperationalCount (optional)
		string operationalCount = ofetchWSS(sink, hDBHandle, hObject, "OperationalCount");
		if (operationalCount != "")
			whsr +=
			"<ibml:OperationalCount>" +
			operationalCount +
			"</ibml:OperationalCount>";

		// TotalQuantity (optional)
		string totalQuantity = ofetchWSS(sink, hDBHandle, hObject, "TotalQuantity");
		if (totalQuantity != "")
			whsr +=
			"<ibml:TotalQuantity>" +
			totalQuantity +
			"</ibml:TotalQuantity>";

		// OnhadnQuantity (optional)
		string onHandQuantity = ofetchWSS(sink, hDBHandle, hObject, "OnHandQuantity");
		if (onHandQuantity != "")
			whsr +=
			"<ibml:OnHandQuantity>" +
			onHandQuantity +
			"</ibml:OnHandQuantity>";

		// RequiredOnHandQuantity (optional)
		string requiredTotalQuantity = ofetchWSS(sink, hDBHandle, hObject, "RequiredTotalQuantity");
		if (requiredTotalQuantity != "")
			whsr +=
			"<ibml:RequiredTotalQuantity>" +
			requiredTotalQuantity +
			"</ibml:RequiredTotalQuantity>";

		// RequiredOnHandQuantity (optional)
		string requiredOnHandQuantity = ofetchWSS(sink, hDBHandle, hObject, "RequiredOnHandQuantity");
		if (requiredOnHandQuantity != "")
			whsr +=
			"<ibml:RequiredOnHandQuantity>" +
			requiredOnHandQuantity +
			"</ibml:RequiredOnHandQuantity>";

		// RequiredCalculationCode (optional)
		string requiredCalculationMethodCode = ofetchWSS(sink, hDBHandle, hObject, "RequiredCalculationMethodCode");
		if (requiredCalculationMethodCode != "")
			whsr +=
			"<ibml:RequiredCalculationMethodCode>" +
			requiredCalculationMethodCode +
			"</ibml:RequiredCalculationMethodCode>";

		whsr += "</ibml:Holding>";

		// Context (optional)
		string context = ofetchWSS(sink, hDBHandle, hObject, "Context");
		if (context != "")
			whsr +=
			"<ibml:Context>" +
			context +
			"</ibml:Context>";

		// ReportID
		whsr +=
			"<ibml:ReportID>" +
			reportID +
			"</ibml:ReportID>";

		bml += whsr;
	}
	/*
	<xs:element name="ControlFeatureReport">
	<xs:complexType>
	<xs:sequence>
	<xs:element name="ReporterWho" type="bml:WhoType"/>
	<xs:element name="Context" type="bml:LabelType" minOccurs="0"/>
	<xs:element name="Hostility" type="mip:ObjectItemHostilityStatusCodeXmlType"/>
	<xs:element name="ControlFeatureID" type="bml:LabelType"/>
	<xs:element name="TypeCategoryCode" type="mip:ControlFeatureTypeCategoryCodeXmlType"/>
	<xs:element name="OpStatus" type="mip:OrganisationStatusOperationalStatusCodeXmlType"/>
	<xs:element name="WhereLocation" type="bml:AtWhereLocationType"/>
	<xs:element name="WhoRef" type="bml:WhoType" minOccurs="0" maxOccurs="unbounded">
	</xs:element>
	<xs:element name="When" type="mip:Datetime18XmlType"/>
	<xs:element name="ReportID" type="bml:LabelType"/>
	<xs:element name="Credibility" type="bml:CredibilityType"/>
	</xs:sequence>
	</xs:complexType>
	</xs:element>
	*/
	else if (reportType == "ControlFeatureReport")
	{
		print(w_submitter, thisObject, "can't generate IBML ControlFeatureReport");
		string cfr;

		bml += cfr;
	}
	/*
	<xs:element name="FacilityStatusReport">
	<xs:complexType>
	<xs:sequence>
	<xs:element name="ReporterWho" type="bml:WhoType"/>
	<xs:element name="Context" type="bml:LabelType" minOccurs="0"/>
	<xs:element name="Hostility" type="mip:ObjectItemHostilityStatusCodeXmlType"/>
	<xs:element name="FacilityID" type="bml:LabelType"/>
	<xs:element name="TypeCategoryCode" type="mip:FacilityTypeCategoryCodeXmlType"/>
	<xs:element name="OpStatus" type="mip:FacilityStatusOperationalStatusQualifierCodeXmlType"/>
	<xs:element name="WhereLocation" type="bml:AtWhereLocationType"/>
	<xs:element name="ResponsibleWho" type="bml:WhoType" minOccurs="0" maxOccurs="unbounded"/>
	<xs:element name="When" type="mip:Datetime18XmlType"/>
	<xs:element name="ReportID" type="bml:LabelType"/>
	<xs:element name="Credibility" type="bml:CredibilityType"/>
	</xs:sequence>
	</xs:complexType>
	</xs:element>
	*/
	else if (reportType == "FacilityStatusReport")
	{
		print(w_submitter, thisObject, "can't generate IBML FacilityStatusReport");
		string fsr;

		bml += fsr;
	}

	// ReportType not found
	else return printError(w_submitter, thisObject, "requested ReportType not recognized:", reportType);

	string s_submitter = ascii(w_submitter);

	// find Forwarder
	wstring w_forwarders;
	CHECK_WISE_RESULT_EX(sink->GetAttributeHandle(hDBHandle, hObject, L"Forwarder", hAttribute));
	CHECK_WISE_RESULT_EX(sink->GetAttributeValue(hDBHandle, hObject, hAttribute, w_forwarders));
	string s_forwarders = ascii(w_forwarders);

	// Closing tags
	bml +=
		"</ibml:" +
		insertReportType +
		"></ibml:StatusReport></ibml:Report></ibml:BMLReport>\r\n";

	// return the generated report
	int bmlCharLength = bml.length() + 1;
	char* returnBml = (char*)malloc(bmlCharLength);
	strncpy_s(returnBml, bmlCharLength, bml.c_str(), bmlCharLength);
	return returnBml;

}	// end generateIBMLReportQuery()


	// generate a string with geo coordinates for IBML Report
string generateIBMLReportQueryGeoCoords(
	char* fetchError,
	wstring w_submitter,
	CWISEDriver* thisObject,
	IWISEDriverSink* sink,
	WISE_HANDLE hDBHandle,
	WISE_HANDLE hObject)
{
	string geoCoords =
		"<ibml:WhereLocation><bml:GDC><bml:Latitude>" +
		fetchWSS(fetchError, w_submitter, thisObject, sink, hDBHandle, hObject, "Latitude") +
		"</bml:Latitude><bml:Longitude>" +
		fetchWSS(fetchError, w_submitter, thisObject, sink, hDBHandle, hObject, "Longitude") +
		"</bml:Longitude>";
	string elevation = fetchWSS(fetchError, w_submitter, thisObject, sink, hDBHandle, hObject, "Elevation");
	if (elevation != "")
		geoCoords +=
		"<bml:ElevationAGL>" + elevation + "</bml:ElevationAGL>";
	geoCoords +=
		"</bml:GDC></ibml:WhereLocation>";

	return geoCoords;

}// end generateIBMLReportQueryGeoCoords()


