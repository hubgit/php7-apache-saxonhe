#ifndef __linux__ 
#ifndef __APPLE__
	//#include "stdafx.h"
	#include <Tchar.h>
#endif
#endif


//#include "stdafx.h"
#include "SaxonProcessor.h"
#include "XdmValue.h"
#include "XdmItem.h"
#include "XdmNode.h"
#include "XdmAtomicValue.h"

#ifndef CPP_ONLY
/*#include "XsltProcessor.cpp"
#include "XQueryProcessor.cpp"
#include "XdmValue.cpp"*/
//#include "php_saxon.cpp"
#endif
//#define DEBUG
#ifdef DEBUG
#include <signal.h>
#endif
#include <stdio.h>


//jobject cpp;
const char * failure;
sxnc_environment * SaxonProcessor::sxn_environ = 0;
int SaxonProcessor::refCount = 0;
int SaxonProcessor::jvmCreatedCPP=0;

bool SaxonProcessor::exceptionOccurred(){
	bool found = SaxonProcessor::sxn_environ->env->ExceptionCheck();
	if(!found){
		if( exception != NULL){
		bool result =  exception->count() > 1;
		return result;
		} else {return false;}
	} else {
		return found;
	}
}

const char* SaxonProcessor::checkException(jobject cpp) {
		const char * message = NULL;		
		if(exception == NULL) {
		  message = checkForException(*sxn_environ, cpp);
	 	} else {
			message = exception->getErrorMessages();	
		}
		return message;
	}

void SaxonProcessor::checkAndCreateException(jclass cppClass){
		exception = NULL;
		if(exceptionOccurred()) {
			if(exception != NULL) {
				delete exception;
			}
		exception = checkForExceptionCPP(SaxonProcessor::sxn_environ->env, cppClass, NULL);
#ifdef DEBUG
		SaxonProcessor::sxn_environ->env->ExceptionDescribe();
#endif
		exceptionClear(false);
		}
	}

void SaxonProcessor::exceptionClear(bool clearCPPException){
	SaxonProcessor::sxn_environ->env->ExceptionClear();
	if(exception != NULL && clearCPPException) {
		delete exception;
	}
}

SaxonApiException * SaxonProcessor::getException(){
	return exception;
}

SaxonProcessor::SaxonProcessor() {
    licensei = false;
}



SaxonApiException * SaxonProcessor::checkForExceptionCPP(JNIEnv* env, jclass callingClass,  jobject callingObject){

    if(exception != NULL) {
	delete exception;	
	}
    if (env->ExceptionCheck()) {
	string result1 = "";
	string errorCode = "";
	jthrowable exc = env->ExceptionOccurred();

#ifdef DEBUG	
	env->ExceptionDescribe();
#endif
	 jclass exccls(env->GetObjectClass(exc));
        jclass clscls(env->FindClass("java/lang/Class"));

        jmethodID getName(env->GetMethodID(clscls, "getName", "()Ljava/lang/String;"));
        jstring name(static_cast<jstring>(env->CallObjectMethod(exccls, getName)));
        char const* utfName(env->GetStringUTFChars(name, 0));
	result1 = (string(utfName));
	//env->ReleaseStringUTFChars(name, utfName);

	 jmethodID  getMessage(env->GetMethodID(exccls, "getMessage", "()Ljava/lang/String;"));
	if(getMessage) {

		jstring message(static_cast<jstring>(env->CallObjectMethod(exc, getMessage)));
		char const* utfMessage = NULL;		
		if(!message) {
			utfMessage = "";
			return NULL;
		} else {
        		utfMessage = (env->GetStringUTFChars(message, 0));
		}
		if(utfMessage != NULL) {
			result1 = (result1 + " : ") + utfMessage;
		} 
		
		//env->ReleaseStringUTFChars(message,utfMessage);
		if(callingObject != NULL && result1.compare(0,36, "net.sf.saxon.option.cpp.SaxonCException", 36) == 0){
			jmethodID  getErrorCodeID(env->GetMethodID(callingClass, "getExceptions", "()[Lnet/sf/saxon/option/cpp/SaxonCException;"));
			jclass saxonExceptionClass(env->FindClass("net/sf/saxon/option/cpp/SaxonCException"));
				if(getErrorCodeID){	
					jobjectArray saxonExceptionObject((jobjectArray)(env->CallObjectMethod(callingObject, getErrorCodeID)));
					if(saxonExceptionObject) {
						jmethodID lineNumID = env->GetMethodID(saxonExceptionClass, "getLinenumber", "()I");
						jmethodID ecID = env->GetMethodID(saxonExceptionClass, "getErrorCode", "()Ljava/lang/String;");
						jmethodID emID = env->GetMethodID(saxonExceptionClass, "getErrorMessage", "()Ljava/lang/String;");
						jmethodID typeID = env->GetMethodID(saxonExceptionClass, "isTypeError", "()Z");
						jmethodID staticID = env->GetMethodID(saxonExceptionClass, "isStaticError", "()Z");
						jmethodID globalID = env->GetMethodID(saxonExceptionClass, "isGlobalError", "()Z");


						int exLength = (int)env->GetArrayLength(saxonExceptionObject);
						SaxonApiException * saxonExceptions = new SaxonApiException();
						for(int i=0; i<exLength;i++){
							jobject exObj = env->GetObjectArrayElement(saxonExceptionObject, i);

							jstring errCode = (jstring)(env->CallObjectMethod(exObj, ecID));
							jstring errMessage = (jstring)(env->CallObjectMethod(exObj, emID));
							jboolean isType = (env->CallBooleanMethod(exObj, typeID));
							jboolean isStatic = (env->CallBooleanMethod(exObj, staticID));
							jboolean isGlobal = (env->CallBooleanMethod(exObj, globalID));
							saxonExceptions->add((errCode ? env->GetStringUTFChars(errCode,0) : NULL )  ,(errMessage ? env->GetStringUTFChars(errMessage,0) : NULL),(int)(env->CallIntMethod(exObj, lineNumID)), (bool)isType, (bool)isStatic, (bool)isGlobal);
							//env->ExceptionDescribe();
						}
						//env->ExceptionDescribe();
						env->ExceptionClear();
						return saxonExceptions;
					}
				}
		}
	}
	SaxonApiException * saxonExceptions = new SaxonApiException(NULL, result1.c_str());
	//env->ExceptionDescribe();
	env->ExceptionClear();
	return saxonExceptions;
     }
	return NULL;

}



SaxonProcessor::SaxonProcessor(bool l){

    cwd="";
    licensei = l;
    versionStr = NULL;
    SaxonProcessor::refCount++;
    exception = NULL;

     if(SaxonProcessor::jvmCreatedCPP == 0){
	SaxonProcessor::jvmCreatedCPP=1;
    SaxonProcessor::sxn_environ= new sxnc_environment;//(sxnc_environment *)malloc(sizeof(sxnc_environment));


    /*
     * First of all, load required component.
     * By the time of JET initialization, all components should be loaded.
     */

    SaxonProcessor::sxn_environ->myDllHandle = loadDefaultDll ();

    /*
     * Initialize JET run-time.
     * The handle of loaded component is used to retrieve Invocation API.
     */
    initDefaultJavaRT (&SaxonProcessor::sxn_environ); 
    } else {
#ifdef DEBUG
     std::cerr<<"SaxonProc constructor: jvm exists!"<<std::endl;
#endif

}

 
    versionClass = lookForClass(SaxonProcessor::sxn_environ->env, "net/sf/saxon/Version");
    procClass = lookForClass(SaxonProcessor::sxn_environ->env, "net/sf/saxon/s9api/Processor");
    saxonCAPIClass = lookForClass(SaxonProcessor::sxn_environ->env, "net/sf/saxon/option/cpp/SaxonCAPI");
    
    proc = createSaxonProcessor (SaxonProcessor::sxn_environ->env, procClass, "(Z)V", NULL, licensei);
	if(!proc) {
		std::cout<<"proc is NULL in SaxonProcessor constructor"<<std::endl;
	}

    xdmAtomicClass = lookForClass(SaxonProcessor::sxn_environ->env, "net/sf/saxon/s9api/XdmAtomicValue");
#ifdef DEBUG
jmethodID debugMID = SaxonProcessor::sxn_environ->env->GetStaticMethodID(saxonCAPIClass, "setDebugMode", "(Z)V");
	SaxonProcessor::sxn_environ->env->CallStaticVoidMethod(saxonCAPIClass, debugMID, (jboolean)true);
#endif
}

SaxonProcessor::SaxonProcessor(const char * configFile){
    cwd="";
    versionStr = NULL;
    SaxonProcessor::refCount++;
    exception = NULL;

    if(SaxonProcessor::jvmCreatedCPP == 0){
	SaxonProcessor::jvmCreatedCPP=1;
    //SaxonProcessor::sxn_environ= new sxnc_environment;
	SaxonProcessor::sxn_environ= (sxnc_environment *)malloc(sizeof(sxnc_environment));

    /*
     * First of all, load required component.
     * By the time of JET initialization, all components should be loaded.
     */

    SaxonProcessor::sxn_environ->myDllHandle = loadDefaultDll ();

    /*
     * Initialize JET run-time.
     * The handle of loaded component is used to retrieve Invocation API.
     */
    initDefaultJavaRT (&SaxonProcessor::sxn_environ); 
    }
 
    versionClass = lookForClass(SaxonProcessor::sxn_environ->env, "net/sf/saxon/Version");

    procClass = lookForClass(SaxonProcessor::sxn_environ->env, "net/sf/saxon/s9api/Processor");
    saxonCAPIClass = lookForClass(SaxonProcessor::sxn_environ->env, "net/sf/saxon/option/cpp/SaxonCAPI");

     static jmethodID mIDcreateProc = (jmethodID)SaxonProcessor::sxn_environ->env->GetStaticMethodID(saxonCAPIClass,"createSaxonProcessor",
					"(Ljava/lang/String;)Lnet/sf/saxon/s9api/Processor;");
		if (!mIDcreateProc) {
			std::cerr << "Error: SaxonDll." << "getPrimitiveTypeName"
				<< " not found\n" << std::endl;
			return ;
		}
	proc = SaxonProcessor::sxn_environ->env->CallStaticObjectMethod(saxonCAPIClass, mIDcreateProc,SaxonProcessor::sxn_environ->env->NewStringUTF(configFile));
		
	if(!proc) {
		checkAndCreateException(saxonCAPIClass);
		std::cerr << "Error: "<<getDllname() << ". processor is NULL in constructor(configFile)"<< std::endl;
		return ;	
	}
	
     licensei = true;
#ifdef DEBUG

     std::cerr<<"SaxonProc constructor(configFile)"<<std::endl;
#endif
    xdmAtomicClass = lookForClass(SaxonProcessor::sxn_environ->env, "net/sf/saxon/s9api/XdmAtomicValue");
}

    SaxonProcessor::~SaxonProcessor(){
	clearConfigurationProperties();
	if(versionStr != NULL) {
		delete versionStr;
	}
	SaxonProcessor::refCount--;	//This might be redundant due to the bug fix 2670
   }



void SaxonProcessor::applyConfigurationProperties(){
	if(configProperties.size()>0) {
		int size = configProperties.size();
		jclass stringClass = lookForClass(SaxonProcessor::sxn_environ->env, "java/lang/String");
		jobjectArray stringArray1 = SaxonProcessor::sxn_environ->env->NewObjectArray( (jint)size, stringClass, 0 );
		jobjectArray stringArray2 = SaxonProcessor::sxn_environ->env->NewObjectArray( (jint)size, stringClass, 0 );
		static jmethodID mIDappConfig = NULL;
		if(mIDappConfig == NULL) {
			mIDappConfig = (jmethodID) SaxonProcessor::sxn_environ->env->GetStaticMethodID(saxonCAPIClass,"applyToConfiguration",
					"(Lnet/sf/saxon/s9api/Processor;[Ljava/lang/String;[Ljava/lang/String;)V");
			if (!mIDappConfig) {
				std::cerr << "Error: SaxonDll." << "applyToConfiguration"
				<< " not found\n" << std::endl;
				return;
			}
		}
		int i=0;
		for(map<std::string, std::string >::iterator iter=configProperties.begin(); iter!=configProperties.end(); ++iter, i++) {
	     		SaxonProcessor::sxn_environ->env->SetObjectArrayElement( stringArray1, i, SaxonProcessor::sxn_environ->env->NewStringUTF( (iter->first).c_str()  ));
	     		SaxonProcessor::sxn_environ->env->SetObjectArrayElement( stringArray2, i, SaxonProcessor::sxn_environ->env->NewStringUTF((iter->second).c_str()) );
	   }
		SaxonProcessor::sxn_environ->env->CallStaticObjectMethod(saxonCAPIClass, mIDappConfig,proc, stringArray1,stringArray2);
		if (exceptionOccurred()) {
	   		checkAndCreateException(saxonCAPIClass);
			exceptionClear(false);
      		 }
 	  SaxonProcessor::sxn_environ->env->DeleteLocalRef(stringArray1);
	  SaxonProcessor::sxn_environ->env->DeleteLocalRef(stringArray2);
		
	}
}


SaxonProcessor& SaxonProcessor::operator=( const SaxonProcessor& other ){
	versionClass = other.versionClass;
	procClass = other.procClass;
	saxonCAPIClass = other.saxonCAPIClass;
	cwd = other.cwd;
	proc = other.proc;
	//SaxonProcessor::sxn_environ= other.environ;
	parameters = other.parameters;
	configProperties = other.configProperties;
	licensei = other.licensei;
	exception = other.exception;
	return *this;
}

XsltProcessor * SaxonProcessor::newXsltProcessor(){
    applyConfigurationProperties();
    return (new XsltProcessor(this, cwd));
}

XQueryProcessor * SaxonProcessor::newXQueryProcessor(){
    applyConfigurationProperties();
    return (new XQueryProcessor(this,cwd));
}

XPathProcessor * SaxonProcessor::newXPathProcessor(){
    applyConfigurationProperties();
    return (new XPathProcessor(this, cwd));
}

SchemaValidator * SaxonProcessor::newSchemaValidator(){
	if(licensei) {
 		applyConfigurationProperties();
		return (new SchemaValidator(this, cwd));
	} else {
		std::cerr<<"\nError: Processor is not licensed for schema processing!"<<std::endl;
		return NULL;
	}
}



const char * SaxonProcessor::version() {
     if(versionStr == NULL) {
     	static jmethodID MID_version = (jmethodID)SaxonProcessor::sxn_environ->env->GetStaticMethodID(saxonCAPIClass, "getProductVersion", "(Lnet/sf/saxon/s9api/Processor;)Ljava/lang/String;");
    	if (!MID_version) {
        	std::cerr<<"\nError: MyClassInDll "<<"SaxonCAPI.getProductVersion()"<<" not found"<<std::endl;
        	return NULL;
    	}

    	jstring jstr = (jstring)(SaxonProcessor::sxn_environ->env->CallStaticObjectMethod(saxonCAPIClass, MID_version, proc));
         const char * tempVersionStr = SaxonProcessor::sxn_environ->env->GetStringUTFChars(jstr, NULL);
         int verLen = strlen(tempVersionStr)+22+strlen(CVERSION)+1;
         versionStr =new char [verLen];
         snprintf(versionStr, verLen, "Saxon/C %s %s %s", CVERSION, "running with", tempVersionStr);
         delete tempVersionStr;

    }
    return versionStr;
}

void SaxonProcessor::setcwd(const char* dir){
    cwd = std::string(dir);
}

void SaxonProcessor::setResourcesDirectory(const char* dir){
	//memset(&resources_dir[0], 0, sizeof(resources_dir));
	strncat(_getResourceDirectory(), dir, strlen(dir));
}


void SaxonProcessor::setCatalog(const char* catalogFile, bool isTracing){
	jclass xmlResolverClass = lookForClass(SaxonProcessor::sxn_environ->env, "Lnet/sf/saxon/trans/XmlCatalogResolver;");
	static jmethodID catalogMID = SaxonProcessor::sxn_environ->env->GetStaticMethodID(xmlResolverClass, "setCatalog", "(Ljava/lang/String;Lnet/sf/saxon/Configuration;Z)V");
	
	if (!catalogMID) {
		std::cerr<<"\nError: Saxonc."<<"setCatalog()"<<" not found"<<std::endl;
        return;
        }
	if(catalogFile == NULL) {
		
		return;
	}
	static jmethodID configMID = SaxonProcessor::sxn_environ->env->GetMethodID(procClass, "getUnderlyingConfiguration", "()Lnet/sf/saxon/Configuration;");
	
	if (!configMID) {
		std::cerr<<"\nError: Saxonc."<<"getUnderlyingConfiguration()"<<" not found"<<std::endl;
        return;
        }


	if(!proc) {
		std::cout<<"proc is NULL in SaxonProcessorsetCatalog"<<std::endl;
		return;
	}

 	jobject configObj = SaxonProcessor::sxn_environ->env->CallObjectMethod(proc, configMID);
  	
	if(!configObj) {
		std::cout<<"proc is NULL in SaxonProcessor setcatalog - config obj"<<std::endl;
		return;
	}
	SaxonProcessor::sxn_environ->env->CallStaticVoidMethod(xmlResolverClass, catalogMID, SaxonProcessor::sxn_environ->env->NewStringUTF(catalogFile), configObj ,(jboolean)isTracing);
#ifdef DEBUG
	SaxonProcessor::sxn_environ->env->ExceptionDescribe();
#endif
}


const char * SaxonProcessor::getResourcesDirectory(){
	return _getResourceDirectory();
}


XdmNode * SaxonProcessor::parseXmlFromString(const char* source){
	
    jmethodID mID = (jmethodID)SaxonProcessor::sxn_environ->env->GetStaticMethodID(saxonCAPIClass, "parseXmlString", "(Lnet/sf/saxon/s9api/Processor;Lnet/sf/saxon/s9api/SchemaValidator;Ljava/lang/String;)Lnet/sf/saxon/s9api/XdmNode;");
    if (!mID) {
	cerr<<"\nError: MyClassInDll "<<"parseXmlString()"<<" not found"<<endl;
        return NULL;
    }
//TODO SchemaValidator

   jobject xdmNodei = SaxonProcessor::sxn_environ->env->CallStaticObjectMethod(saxonCAPIClass, mID, proc, NULL, SaxonProcessor::sxn_environ->env->NewStringUTF(source));
	if(xdmNodei) {
		XdmNode * value = new XdmNode(xdmNodei);
		value->setProcessor(this);
		return value;
	}   else if (exceptionOccurred()) {
	   	checkAndCreateException(saxonCAPIClass);
		exceptionClear(false);
       }
   
#ifdef DEBUG
	SaxonProcessor::sxn_environ->env->ExceptionDescribe();
#endif
 
   return NULL;
}

int SaxonProcessor::getNodeKind(jobject obj){
	jclass xdmNodeClass = lookForClass(SaxonProcessor::sxn_environ->env, "Lnet/sf/saxon/s9api/XdmNode;");
	static jmethodID nodeKindMID = (jmethodID) SaxonProcessor::sxn_environ->env->GetMethodID(xdmNodeClass,"getNodeKind", "()Lnet/sf/saxon/s9api/XdmNodeKind;");
	if (!nodeKindMID) {
		cerr << "Error: MyClassInDll." << "getNodeKind" << " not found\n"
				<< endl;
		return 0;
	} 

	jobject nodeKindObj = (SaxonProcessor::sxn_environ->env->CallObjectMethod(obj, nodeKindMID));
	if(!nodeKindObj) {
		
		return 0;
	}
	jclass xdmUtilsClass = lookForClass(SaxonProcessor::sxn_environ->env, "Lnet/sf/saxon/option/cpp/XdmUtils;");

	jmethodID mID2 = (jmethodID) SaxonProcessor::sxn_environ->env->GetStaticMethodID(xdmUtilsClass,"convertNodeKindType", "(Lnet/sf/saxon/s9api/XdmNodeKind;)I");

	if (!mID2) {
		cerr << "Error: MyClassInDll." << "convertNodeKindType" << " not found\n"
				<< endl;
		return 0;
	} 
	if(!nodeKindObj){
		return 0;	
	}
	int nodeKind = (long)(SaxonProcessor::sxn_environ->env->CallStaticObjectMethod(xdmUtilsClass, mID2, nodeKindObj));
	return nodeKind;
}

XdmNode * SaxonProcessor::parseXmlFromFile(const char* source){

    jmethodID mID = (jmethodID)SaxonProcessor::sxn_environ->env->GetStaticMethodID(saxonCAPIClass, "parseXmlFile", "(Lnet/sf/saxon/s9api/Processor;Ljava/lang/String;Lnet/sf/saxon/s9api/SchemaValidator;Ljava/lang/String;)Lnet/sf/saxon/s9api/XdmNode;");
    if (!mID) {
	cerr<<"\nError: MyClassInDll "<<"parseXmlFile()"<<" not found"<<endl;
        return NULL;
    }
//TODO SchemaValidator
   jobject xdmNodei = SaxonProcessor::sxn_environ->env->CallStaticObjectMethod(saxonCAPIClass, mID, proc, SaxonProcessor::sxn_environ->env->NewStringUTF(cwd.c_str()),  NULL, SaxonProcessor::sxn_environ->env->NewStringUTF(source));
     if(exceptionOccurred()) {
	 	checkAndCreateException(saxonCAPIClass);
	   exceptionClear(false);
	   		
     } else {

	XdmNode * value = new XdmNode(xdmNodei);
	value->setProcessor(this);
	return value;
   }
   return NULL;
}

XdmNode * SaxonProcessor::parseXmlFromUri(const char* source){

    jmethodID mID = (jmethodID)SaxonProcessor::sxn_environ->env->GetStaticMethodID(saxonCAPIClass, "parseXmlFile", "(Lnet/sf/saxon/s9api/Processor;Ljava/lang/String;Ljava/lang/String;)Lnet/sf/saxon/s9api/XdmNode;");
    if (!mID) {
	cerr<<"\nError: MyClassInDll "<<"parseXmlFromUri()"<<" not found"<<endl;
        return NULL;
    }
   jobject xdmNodei = SaxonProcessor::sxn_environ->env->CallStaticObjectMethod(saxonCAPIClass, mID, proc, SaxonProcessor::sxn_environ->env->NewStringUTF(""), SaxonProcessor::sxn_environ->env->NewStringUTF(source));
     if(exceptionOccurred()) {
	   checkAndCreateException(saxonCAPIClass);
     } else {
	XdmNode * value = new XdmNode(xdmNodei);
	value->setProcessor(this);
	return value;
   }
   return NULL;
}


  /**
     * Set a configuration property.
     *
     * @param name of the property
     * @param value of the property
     */
    void SaxonProcessor::setConfigurationProperty(const char * name, const char * value){
	if(name != NULL){
		configProperties.insert(std::pair<std::string, std::string>(std::string(name), std::string((value == NULL ? "" : value))));
	}
    }

   void SaxonProcessor::clearConfigurationProperties(){
	configProperties.clear();
   }



void SaxonProcessor::release(){
	if(SaxonProcessor::jvmCreatedCPP!=0) {
		SaxonProcessor::jvmCreatedCPP =0;

 		finalizeJavaRT (SaxonProcessor::sxn_environ->jvm);

		//delete SaxonProcessor::sxn_environ;
	/*clearParameters();
	clearProperties();*/
} else {
#ifdef DEBUG
     std::cerr<<"SaxonProc: JVM finalize not called!"<<std::endl;
#endif
}
}




/* ========= Factory method for Xdm ======== */

    XdmAtomicValue * SaxonProcessor::makeStringValue(std::string str){
	jobject obj = getJavaStringValue(*SaxonProcessor::sxn_environ, str.c_str());
	jmethodID mID_atomic = (jmethodID)(SaxonProcessor::sxn_environ->env->GetMethodID (xdmAtomicClass, "<init>", "(Ljava/lang/String;)V"));
	jobject obj2 = (jobject)(SaxonProcessor::sxn_environ->env->NewObject(xdmAtomicClass, mID_atomic, obj));
	XdmAtomicValue * value = new XdmAtomicValue(obj2, "xs:string");
	value->setProcessor(this);
	return value;
    }

    XdmAtomicValue * SaxonProcessor::makeStringValue(const char * str){
	jobject obj = getJavaStringValue(*SaxonProcessor::sxn_environ, str);
	jmethodID mID_atomic = (jmethodID)(SaxonProcessor::sxn_environ->env->GetMethodID (xdmAtomicClass, "<init>", "(Ljava/lang/String;)V"));
	jobject obj2 = (jobject)(SaxonProcessor::sxn_environ->env->NewObject(xdmAtomicClass, mID_atomic, obj));
	XdmAtomicValue * value = new XdmAtomicValue(obj2, "xs:string");
	value->setProcessor(this);
	return value;
    }

    XdmAtomicValue * SaxonProcessor::makeIntegerValue(int i){
	//jobject obj = integerValue(*SaxonProcessor::sxn_environ, i);
	jmethodID mID_atomic = (jmethodID)(SaxonProcessor::sxn_environ->env->GetMethodID (xdmAtomicClass, "<init>", "(J)V"));
	

	jobject obj = (jobject)(SaxonProcessor::sxn_environ->env->NewObject(xdmAtomicClass, mID_atomic, (jlong)i));
	XdmAtomicValue * value = new XdmAtomicValue(obj, "Q{http://www.w3.org/2001/XMLSchema}integer");
	value->setProcessor(this);
	return value;
    }

    XdmAtomicValue * SaxonProcessor::makeDoubleValue(double d){
	//jobject obj = doubleValue(*SaxonProcessor::sxn_environ, d);
	jmethodID mID_atomic = (jmethodID)(SaxonProcessor::sxn_environ->env->GetMethodID (xdmAtomicClass, "<init>", "(D)V"));
	jobject obj = (jobject)(SaxonProcessor::sxn_environ->env->NewObject(xdmAtomicClass, mID_atomic, (jdouble)d));
	XdmAtomicValue * value = new XdmAtomicValue(obj, "Q{http://www.w3.org/2001/XMLSchema}double");
	value->setProcessor(this);
	return value;
    }

    XdmAtomicValue * SaxonProcessor::makeFloatValue(float d){
	//jobject obj = doubleValue(*SaxonProcessor::sxn_environ, d);
	jmethodID mID_atomic = (jmethodID)(SaxonProcessor::sxn_environ->env->GetMethodID (xdmAtomicClass, "<init>", "(F)V"));
	jobject obj = (jobject)(SaxonProcessor::sxn_environ->env->NewObject(xdmAtomicClass, mID_atomic, (jfloat)d));
	XdmAtomicValue * value = new XdmAtomicValue(obj, "Q{http://www.w3.org/2001/XMLSchema}float");
	value->setProcessor(this);
	return value;
    }

    XdmAtomicValue * SaxonProcessor::makeLongValue(long l){
	//jobject obj = longValue(*SaxonProcessor::sxn_environ, l);
	jmethodID mID_atomic = (jmethodID)(SaxonProcessor::sxn_environ->env->GetMethodID (xdmAtomicClass, "<init>", "(J)V"));
	jobject obj = (jobject)(SaxonProcessor::sxn_environ->env->NewObject(xdmAtomicClass, mID_atomic, (jlong)l));
	XdmAtomicValue * value = new XdmAtomicValue(obj, "Q{http://www.w3.org/2001/XMLSchema}long");
	value->setProcessor(this);
	return value;
    }

    XdmAtomicValue * SaxonProcessor::makeBooleanValue(bool b){
	//jobject obj = booleanValue(*SaxonProcessor::sxn_environ, b);
	jmethodID mID_atomic = (jmethodID)(SaxonProcessor::sxn_environ->env->GetMethodID (xdmAtomicClass, "<init>", "(Z)V"));
	jobject obj = (jobject)(SaxonProcessor::sxn_environ->env->NewObject(xdmAtomicClass, mID_atomic, (jboolean)b));
	XdmAtomicValue * value = new XdmAtomicValue(obj, "Q{http://www.w3.org/2001/XMLSchema}boolean");
	value->setProcessor(this);
	return value;
    }

    XdmAtomicValue * SaxonProcessor::makeQNameValue(std::string str){
	jobject val = xdmValueAsObj(*SaxonProcessor::sxn_environ, "QName", str.c_str());
	XdmAtomicValue * value = new XdmAtomicValue(val, "QName");
	value->setProcessor(this);
	return value;
    }

    XdmAtomicValue * SaxonProcessor::makeAtomicValue(std::string typei, std::string strValue){
	jobject obj = xdmValueAsObj(*SaxonProcessor::sxn_environ, typei.c_str(), strValue.c_str());
	XdmAtomicValue * value = new XdmAtomicValue(obj, typei);
	value->setProcessor(this);
	return value;
    }

    const char * SaxonProcessor::getStringValue(XdmItem * item){
	const char *result = stringValue(*SaxonProcessor::sxn_environ, item->getUnderlyingValue());
#ifdef DEBUG
	if(result == NULL) {
		std::cout<<"getStringValue of XdmItem is NULL"<<std::endl;
	} else {
		std::cout<<"getStringValue of XdmItem is OK"<<std::endl;
	}
#endif
	return result;
    }




