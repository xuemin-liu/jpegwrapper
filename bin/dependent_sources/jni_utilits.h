/*
 * jni_utilits.h
 *
 *  Created on: 2015年10月2日
 *      Author: guyadong
 */

#ifndef COMMON_SOURCE_CPP_JNIUTILS_H_
#define COMMON_SOURCE_CPP_JNIUTILS_H_
#include <cstdint>
#include <functional>
#include <type_traits>
#include <stdexcept>
#include <cassert>
#include <string>
#include <vector>
#include <jni.h>
#include <unordered_map>
#include <memory>
#include <initializer_list>
#include "raii.h"
#include "assert_macros.h"
#define jni_throw_except_if_msg(except,expression,msg) \
	if(expression){\
		gdface::jni_utilits::throwByName(except, ERROR_STR(msg));\
		throw gdface::jni_utilits::caused_by_java_except(ERROR_STR(msg));\
	}
#define jni_throw_except_if(except,expression) jni_throw_except_if_msg(except,expression,#expression)
#define jni_throw_if_msg(expression,msg) jni_throw_except_if_msg("java/lang/IllegalArgumentException",expression,msg)
#define jni_throw_if(expression) jni_throw_except_if("java/lang/IllegalArgumentException",expression)
#define jni_throw_except_if_null(except,p) jni_throw_except_if_msg(except,nullptr==p,#p" is null")
#define jni_throw_if_null(p) jni_throw_if_msg(nullptr==p,#p" is null")
namespace gdface{
namespace jni_utilits {
	/*
	* 用于jni接口函数最外层异常捕获
	* 当抛出此异常时，代表有Java异常抛出(JNIEnv::ExceptionOccurred() 为true)
	* 处理此异常不需要做任何事
	* */
	class caused_by_java_except :public std::logic_error {
	public:
		// 继承基类构造函数
		using std::logic_error::logic_error;
		caused_by_java_except(const std::string &at, const std::string &msg) :logic_error(std::string(at).append(":").append(msg)) {}
		caused_by_java_except(const std::string &at, const std::exception&e) :logic_error(std::string(at).append(":").append(e.what())) {}
		caused_by_java_except(const std::string &at, const std::exception&e, const std::string &msg) :logic_error(std::string(at).append(":").append(e.what()).append(":").append(msg)) {}
	};
	void throwByName(const char* name, const char* msg);
	inline void throwByName(const std::string& name, const std::string&msg) {
		throwByName(name.data(), msg.data());
	}
	void throwIllegalArgumentException(const char* msg);
	inline void throwIllegalArgumentException(const std::string& msg) {
		throwIllegalArgumentException(msg.data());
	}
	/* 将一个完整java类名(fully-qualified class name)转为类型签名 */
	inline std::string typeSignature(const std::string& fully_qualified_class_name) {
		return std::string("L").append(fully_qualified_class_name).append(";");
	}
	/* 将一个完整java类名(fully-qualified class name)转为类型签名 */
	inline std::string typeSignature(const char* fully_qualified_class_name) {
		return typeSignature(std::string(fully_qualified_class_name));
	}

	/* 抛出std::logic_error, 并向cerr输出 */
	void throw_fault(const char* msg);
	void setJVM(JavaVM * jvm);
	JNIEnv* getJNIEnv();

	/* raii方式管理F函数生产的jobject对象局部引用
	 * 如果调用时指定T类型，则返回的RAII对象类型为T,否则类型为F(Args...)结果类型
	 */
	template<typename T= jobject,typename F, typename... Args,
			typename ACQ_RES_TYPE=typename std::result_of<F(Args...)>::type,
			typename TYPE=typename std::conditional<!std::is_same<T, jobject>::value&&!std::is_same<T,ACQ_RES_TYPE>::value,T,ACQ_RES_TYPE>::type>
	static raii_var<TYPE>
	raii_jobject_var(F&& f, Args&&... args){
		static_assert(
				std::is_base_of<typename std::remove_pointer<ACQ_RES_TYPE>::type,
										typename std::remove_pointer<TYPE>::type>::value,
				"T is not derived from jobject");
		auto var= raii_bind_var<TYPE>(
				[](TYPE &obj) {if(nullptr!=obj)getJNIEnv()->DeleteLocalRef(obj);},
				std::forward<F>(f), std::forward<Args>(args)...
				);
		auto env=getJNIEnv();
		auto exec=env->ExceptionOccurred();
		if(exec){
			//env->ExceptionClear();
			env->ExceptionDescribe();
			//env->Throw(exec);
		}
		return std::move(var);
	}

	/* RAII方式管理JNIEnv的成员函数 */
	template<typename T=jobject,typename F, typename... Args>
	inline auto
	raii_jobject_env(F&& f, Args&&... args)->decltype(raii_jobject_var<T>(f,getJNIEnv(),std::forward<Args>(args)...)){
		return raii_jobject_var<T>(f,getJNIEnv(),std::forward<Args>(args)...);
	}
	/* 封装JNIEnv::NewGlobalRef 返回jobject对象全局引用(RAII管理) */
	template<typename T=jobject,typename TYPE=typename std::conditional<!std::is_same<T,jobject>::value,T,jobject>::type>
	static raii_var<TYPE>
	raii_NewGlobalRef(T localRef) {
		static_assert(
				std::is_base_of<typename std::remove_pointer<jobject>::type,
										typename std::remove_pointer<T>::type>::value,
				"T is not derived from jobject");
		return raii_var<TYPE>(
			[localRef]()->TYPE {return static_cast<TYPE>(getJNIEnv()->NewGlobalRef(localRef));},
			[](TYPE &gref) {getJNIEnv()->DeleteGlobalRef(gref);});
	}

	/* 封装JNIEnv::FindClass 返回jclass对象局部引用(RAII管理) */
	inline raii_var<jclass> raii_FindClass_LocalRef(const char* name) {
		assert(nullptr != name);
		return raii_jobject_env(&JNIEnv::FindClass, name);
	}

	/* 封装JNIEnv::FindClass 返回jclass对象全局引用(RAII管理) */
	inline raii_var<jclass> raii_FindClass_GlobalRef(const char* name) {
		return raii_NewGlobalRef<jclass>(raii_FindClass_LocalRef(name).get());
	}

	/* 封装JNIEnv::GetObjectClass */
	inline raii_var<jclass> raii_GetObjectClass(jobject obj) {
		assert(nullptr != obj);
		return raii_jobject_env(&JNIEnv::GetObjectClass, obj);
	}
	/* 封装JNIEnv::CallObjectMethod */
	template<typename T=jobject,typename... Args>
	inline raii_var<T> raii_CallObjectMethod( jobject obj, jmethodID methodID, Args&&... args) {
		return raii_jobject_env<T>(&JNIEnv::CallObjectMethod,obj, methodID,std::forward<Args>(args)...);
	}
	template<typename T=jobject,typename... Args>
	inline raii_var<T> raii_CallObjectMethod( jobject obj, const char *name, const char *sig, Args&&... args) {
		raii_var<jclass> clazz=raii_GetObjectClass(obj);
		auto methodID = getJNIEnv()->GetMethodID(clazz.get(), name, sig);
		assert(nullptr != methodID);
		return raii_CallObjectMethod<T>(obj,methodID,std::forward<Args>(args)...);
	}
	template<typename T=jobject,typename... Args>
	inline raii_var<T> raii_NewObject( jclass clazz, jmethodID constructor, Args&&... args) {
		return raii_jobject_env(&JNIEnv::NewObject,clazz,constructor,std::forward<Args>(args)...);
	}
	template<typename T=jobject,typename... Args>
	inline raii_var<T> raii_NewObject( jclass clazz, const char *sig, Args&&... args) {
		assert(nullptr != clazz);
		auto constructor = getJNIEnv()->GetMethodID(clazz, "<init>", nullptr==sig?"void (V)":sig);
		assert(nullptr != constructor);
		return raii_NewObject<T>(clazz,constructor,std::forward<Args>(args)...);
	}
	template<typename T=jobject,typename... Args>
	inline raii_var<T> raii_NewObject( const char * class_name, const char *sig,Args&&... args) {
		return raii_NewObject<T>(raii_FindClass_LocalRef(class_name).get(),sig,std::forward<Args>(args)...);
	}
	template<typename T=jobject>
	inline raii_var<T> raii_NewObject( jclass clazz) {
		return raii_NewObject<T>(clazz,(const char *)nullptr);
	}
	template<typename T=jobject>
	inline raii_var<T> raii_NewObject( const char * class_name) {
		return raii_NewObject<T>(raii_FindClass_LocalRef(class_name).get());
	}
	inline raii_var<jbyteArray> raii_NewByteArray( jsize len) {
		return raii_jobject_env(&JNIEnv::NewByteArray, len);
	}
	/* 将bytes转成raii_var<jbyteArray>对象 */
	inline raii_var<jbyteArray> tojbytearray(jbyte* bytes, jsize len) {
		auto byteArray = raii_NewByteArray(len);
		if (nullptr != bytes)
			getJNIEnv()->SetByteArrayRegion(byteArray.get(), 0, len, bytes);
		return byteArray;
	}
	/* 将UTF-8字符串转成一个raii_var管理的JString对象 */
	inline raii_var<jstring> raii_NewStringUTF(const char* pStr) {
		return raii_jobject_env(&JNIEnv::NewStringUTF, pStr);
	}
	/* 将Unicode字符串转成一个raii_var管理的JString对象 */
	inline raii_var<jstring> raii_NewString(const jchar* pStr, jsize len) {
		return raii_jobject_env(&JNIEnv::NewString, pStr, len);
	}

	/* 封装JNIEnv::GetObjectField 返回T指定的jobject基类对象(RAII管理) */
	template<typename T=jobject>
	inline raii_var<T> raii_GetObjectField(jobject obj, jfieldID fieldID) {
		return raii_jobject_env<T>(&JNIEnv::GetObjectField,obj,fieldID);
	}
	/* 封装JNIEnv::GetObjectField 返回T指定的jobject基类对象(RAII管理) */
	template<typename T=jobject>
	inline raii_var<T> raii_GetStaticObjectField(jclass clazz, jfieldID fieldID) {
		return raii_jobject_env<T>(&JNIEnv::GetStaticObjectField,clazz,fieldID);
	}
	/* 封装JNIEnv::GetObjectArrayElement 返回T指定的jobject基类对象(RAII管理) */
	template<typename T=jobject>
	inline raii_var<T> raii_GetObjectArrayElement(jobjectArray array, jsize index) {
		return raii_jobject_env<T>(&JNIEnv::GetObjectArrayElement,array,index);
	}
	/* 封装JNIEnv::NewObjectArray 返回T指定的jobject基类对象(RAII管理) */
	inline raii_var<jobjectArray> raii_NewObjectArray(jsize size,jclass clazz,jobject init) {
		return raii_jobject_env<jobjectArray>(&JNIEnv::NewObjectArray,size,clazz,init);
	}
	/* 封装JNIEnv::GetByteArrayElements 返回字节数组对象(jbyteArray)的内存指针(RAII管理)
	 * release_mode取值 0,JNI_COMMIT,JNI_ABORT参见《Java Native Interface Specification Contents》
	 * http://docs.oracle.com/javase/8/docs/technotes/guides/jni/spec/functions.html#Release_PrimitiveType_ArrayElements_routines
	 * */
	static raii_var<jbyte*> raii_GetByteArrayElements(jbyteArray bytes,	jint release_mode = JNI_ABORT){
		using type = decltype(getJNIEnv()->GetByteArrayElements(bytes, nullptr));
		assert(nullptr != bytes);
		return raii_var<type>(
				[bytes]()->type {
					jboolean isCopy;
					auto result=getJNIEnv()->GetByteArrayElements(bytes, &isCopy);
					assert(isCopy); // 调用成功断言
					return std::move(result);
				},
				[release_mode,bytes](type &obj) {getJNIEnv()->ReleaseByteArrayElements(bytes, obj, release_mode);});
	}	
	/*
	* 从jbyteArray中返回字节数组指针,
	* jbytes为nullptr则抛出异常
	* jbytes的长度必须与limit相等,否则抛出异常,
	* release_mode取值 0,JNI_COMMIT,JNI_ABORT参见《Java Native Interface Specification Contents》
	* http://docs.oracle.com/javase/8/docs/technotes/guides/jni/spec/functions.html#Release_PrimitiveType_ArrayElements_routines
	* 返回分配内存的指针(RAII管理)
	* */
	static raii_var<jbyte*> raii_GetByteArrayElementsLimit(jbyteArray jbytes, jsize limit, jint release_mode = JNI_ABORT) {
		jni_throw_if_msg(nullptr == jbytes, "invalid jbyteArray")
		auto size = jni_utilits::getJNIEnv()->GetArrayLength(jbytes);
		assert(size >= 0);
		jni_throw_if_msg(size != limit, "invalid jbyteArray length")
		return  raii_GetByteArrayElements(jbytes, release_mode);
	}
	static auto raii_GetStringUTFChars(
			jstring jstr) -> raii_var<decltype(getJNIEnv()->GetStringUTFChars(jstr, nullptr))> {
		using type = decltype(getJNIEnv()->GetStringUTFChars(jstr, nullptr));
		assert(nullptr != jstr);
		return raii_var<type>([jstr]()->type {
			jboolean isCopy;
			auto result=getJNIEnv()->GetStringUTFChars(jstr, &isCopy);
			assert(isCopy); // 调用成功断言
				return std::move(result);
			}, [jstr](type &obj) {getJNIEnv()->ReleaseStringUTFChars(jstr,obj);});
	}
	uint8_t * copyToBytePtr(jbyteArray jbytes, jsize limit = -1,jint release_mode= JNI_ABORT);
	std::vector<uint8_t>  copyToVector(jbyteArray jbytes,jsize limit=-1);
	bool jclass_isInstance(jclass clazz,jobject jobj);
	bool jclass_isArray(jclass clazz);
	raii_var<jclass> jclass_getComponentType(jclass clazz);
	raii_var<jstring> jclass_getNameJString(jclass clazz);
	std::string jclass_getName(jclass clazz);
	raii_var<jbyteArray> jstring_getBytes(jstring jstr);
	jint jstring_length(jstring jstr);

	class JavaClassMirror{
	public:
		raii_var<jclass> javaclass;
		jmethodID  constructor;
		std::unordered_map<std::string,jfieldID> field;
		JavaClassMirror() = default;
		JavaClassMirror(std::string canonicalName, std::pair<std::string,std::string> constr, std::initializer_list<std::pair<std::string,std::string> > field_signature);
		JavaClassMirror(JavaClassMirror&&)=default;		
		JavaClassMirror& operator=(JavaClassMirror&&)=default;
		//==========SetField==========
		template<typename T>
		typename std::enable_if<std::is_same<T,jdouble>::value>::type SetField(jobject obj, const char* name, T fieldObj) const{
			jni_utilits::getJNIEnv()->SetDoubleField(obj, field.find(name)->second, fieldObj);
		}
		template<typename T>
		typename std::enable_if<std::is_same<T,jfloat>::value>::type SetField(jobject obj,const char* name,T fieldObj)const{
			jni_utilits::getJNIEnv()->SetFloatField(obj,field.find(name)->second,fieldObj);
		}
		template<typename T>
		typename std::enable_if<std::is_same<T, jlong>::value>::type SetField(jobject obj, const char* name, T fieldObj) const{
			jni_utilits::getJNIEnv()->SetLongField(obj, field.find(name)->second, fieldObj);
		}
		template<typename T>
		typename std::enable_if<std::is_same<T, jint>::value>::type SetField(jobject obj, const char* name, T fieldObj) const{
			jni_utilits::getJNIEnv()->SetIntField(obj, field.find(name)->second, fieldObj);
		}
		template<typename T>
		typename std::enable_if<std::is_same<T, jshort>::value>::type SetField(jobject obj, const char* name, T fieldObj) const{
			jni_utilits::getJNIEnv()->SetShortField(obj, field.find(name)->second, fieldObj);
		}
		template<typename T>
		typename std::enable_if<std::is_same<T, jchar>::value>::type SetField(jobject obj, const char* name, T fieldObj)const {
			jni_utilits::getJNIEnv()->SetCharField(obj, field.find(name)->second, fieldObj);
		}
		template<typename T>
		typename std::enable_if<std::is_same<T, jbyte>::value>::type SetField(jobject obj, const char* name, T fieldObj) const{
			jni_utilits::getJNIEnv()->SetByteField(obj, field.find(name)->second, fieldObj);
		}
		template<typename T>
		typename std::enable_if<std::is_same<T, jboolean>::value>::type SetField(jobject obj, const char* name, T fieldObj)const {
			jni_utilits::getJNIEnv()->SetBooleanField(obj, field.find(name)->second, fieldObj);
		}
		template<typename T>
		typename std::enable_if<std::is_base_of<_jobject,typename std::remove_pointer<T>::type>::value>::type
		SetField(jobject obj, const char* name, T fieldObj)const {
			jni_utilits::getJNIEnv()->SetObjectField(obj, field.find(name)->second, fieldObj);
		}
		//==========GetField==========
		template<typename T>
		typename std::enable_if<std::is_same<T,jdouble>::value,T>::type GetField(jobject obj,const char* name)const{
			return jni_utilits::getJNIEnv()->GetDoubleField(obj,field.find(name)->second);
		}
		template<typename T>
		typename std::enable_if<std::is_same<T,jfloat>::value,T>::type GetField(jobject obj,const char* name)const{
			return jni_utilits::getJNIEnv()->GetFloatField(obj,field.find(name)->second);
		}
		template<typename T>
		typename std::enable_if<std::is_same<T,jlong>::value,T>::type GetField(jobject obj,const char* name)const{
			return jni_utilits::getJNIEnv()->GetLongField(obj,field.find(name)->second);
		}
		template<typename T>
		typename std::enable_if<std::is_same<T,jint>::value,T>::type GetField(jobject obj,const char* name)const{
			return jni_utilits::getJNIEnv()->GetIntField(obj,field.find(name)->second);
		}
		template<typename T>
		typename std::enable_if<std::is_same<T,jshort>::value,T>::type GetField(jobject obj,const char* name)const{
			return jni_utilits::getJNIEnv()->GetShortField(obj,field.find(name)->second);
		}
		template<typename T>
		typename std::enable_if<std::is_same<T,jchar>::value,T>::type GetField(jobject obj,const char* name)const{
			return jni_utilits::getJNIEnv()->GetCharField(obj,field.find(name)->second);
		}
		template<typename T>
		typename std::enable_if<std::is_same<T,jbyte>::value,T>::type GetField(jobject obj,const char* name)const{
			return jni_utilits::getJNIEnv()->GetByteField(obj,field.find(name)->second);
		}
		template<typename T>
		typename std::enable_if<std::is_same<T,jboolean>::value,T>::type GetField(jobject obj,const char* name)const{
			return jni_utilits::getJNIEnv()->GetBooleanField(obj,field.find(name)->second);
		}
		template<typename T>
		typename std::enable_if<std::is_base_of<_jobject,typename std::remove_pointer<T>::type>::value,raii_var<T>>::type
		GetField(jobject obj,const char* name)const{
			return jni_utilits::raii_GetObjectField<T>(obj,field.find(name)->second);
		}
		//==========SetStaticField==========
		template<typename T>
		typename std::enable_if<std::is_same<T,jdouble>::value>::type SetStaticField(const char* name, T fieldObj) const{
			jni_utilits::getJNIEnv()->SetStaticDoubleField(*javaclass, field.find(name)->second, fieldObj);
		}
		template<typename T>
		typename std::enable_if<std::is_same<T,jfloat>::value>::type SetStaticField(const char* name,T fieldObj)const{
			jni_utilits::getJNIEnv()->SetStaticFloatField(*javaclass,field.find(name)->second,fieldObj);
		}
		template<typename T>
		typename std::enable_if<std::is_same<T, jlong>::value>::type SetStaticField(const char* name, T fieldObj) const{
			jni_utilits::getJNIEnv()->SetStaticLongField(*javaclass, field.find(name)->second, fieldObj);
		}
		template<typename T>
		typename std::enable_if<std::is_same<T, jint>::value>::type SetStaticField(const char* name, T fieldObj) const{
			jni_utilits::getJNIEnv()->SetStaticIntField(*javaclass, field.find(name)->second, fieldObj);
		}
		template<typename T>
		typename std::enable_if<std::is_same<T, jshort>::value>::type SetStaticField(const char* name, T fieldObj) const{
			jni_utilits::getJNIEnv()->SetStaticShortField(*javaclass, field.find(name)->second, fieldObj);
		}
		template<typename T>
		typename std::enable_if<std::is_same<T, jchar>::value>::type SetStaticField(const char* name, T fieldObj)const {
			jni_utilits::getJNIEnv()->SetStaticCharField(*javaclass, field.find(name)->second, fieldObj);
		}
		template<typename T>
		typename std::enable_if<std::is_same<T, jbyte>::value>::type SetStaticField(const char* name, T fieldObj) const{
			jni_utilits::getJNIEnv()->SetStaticByteField(*javaclass, field.find(name)->second, fieldObj);
		}
		template<typename T>
		typename std::enable_if<std::is_same<T, jboolean>::value>::type SetStaticField(const char* name, T fieldObj)const {
			jni_utilits::getJNIEnv()->SetStaticBooleanField(*javaclass, field.find(name)->second, fieldObj);
		}
		template<typename T>
		typename std::enable_if<std::is_base_of<_jobject,typename std::remove_pointer<T>::type>::value>::type
		SetStaticField(const char* name, T fieldObj)const {
			jni_utilits::getJNIEnv()->SetStaticObjectField(*javaclass, field.find(name)->second, fieldObj);
		}
		//=========GetStaticField============
		template<typename T>
		typename std::enable_if<std::is_same<T,jdouble>::value,T>::type GetStaticField(const char* name)const{
			return jni_utilits::getJNIEnv()->GetStaticDoubleField(*javaclass,field.find(name)->second);
		}
		template<typename T>
		typename std::enable_if<std::is_same<T,jfloat>::value,T>::type GetStaticField(const char* name)const{
			return jni_utilits::getJNIEnv()->GetStaticFloatField(*javaclass,field.find(name)->second);
		}
		template<typename T>
		typename std::enable_if<std::is_same<T,jlong>::value,T>::type GetStaticField(const char* name)const{
			return jni_utilits::getJNIEnv()->GetStaticLongField(*javaclass,field.find(name)->second);
		}
		template<typename T>
		typename std::enable_if<std::is_same<T,jint>::value,T>::type GetStaticField(const char* name)const{
			return jni_utilits::getJNIEnv()->GetStaticIntField(*javaclass,field.find(name)->second);
		}
		template<typename T>
		typename std::enable_if<std::is_same<T,jshort>::value,T>::type GetStaticField(const char* name)const{
			return jni_utilits::getJNIEnv()->GetStaticShortField(*javaclass,field.find(name)->second);
		}
		template<typename T>
		typename std::enable_if<std::is_same<T,jchar>::value,T>::type GetStaticField(const char* name)const{
			return jni_utilits::getJNIEnv()->GetStaticCharField(*javaclass,field.find(name)->second);
		}
		template<typename T>
		typename std::enable_if<std::is_same<T,jbyte>::value,T>::type GetStaticField(const char* name)const{
			return jni_utilits::getJNIEnv()->GetStaticByteField(*javaclass,field.find(name)->second);
		}
		template<typename T>
		typename std::enable_if<std::is_same<T,jboolean>::value,T>::type GetStaticField(const char* name)const{
			return jni_utilits::getJNIEnv()->GetStaticBooleanField(*javaclass,field.find(name)->second);
		}
		template<typename T>
		typename std::enable_if<std::is_base_of<_jobject,typename std::remove_pointer<T>::type>::value,raii_var<T>>::type
		GetStaticField(const char* name)const{
			return jni_utilits::raii_GetStaticObjectField<T>(*javaclass,field.find(name)->second);
		}
	};/*class JavaClassMirror*/
} /*  namespace jni_utilits */
} /* namespace gdface */
#endif /* COMMON_SOURCE_CPP_JNIUTILS_H_ */
