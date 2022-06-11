/*
 * jni_utilits.cpp
 *
 *  Created on: 2015年10月2日
 *      Author: guyadong
 */
#include <stdexcept>
#include <mutex>
#include <iostream>
#include <cstring>
#include "jni_utilits.h"
namespace gdface {
namespace jni_utilits{
JavaVM * JVM=nullptr;
thread_local JNIEnv* env=nullptr;
void _getJNIEnv() {
	if(nullptr == JVM){
		throw_fault("JNI ERROR:JVM is nullptr");
	}
	auto res=JVM->GetEnv((void **)std::addressof(env), JNI_VERSION_1_6);
	if (res != JNI_OK||nullptr == env){
		throw_fault("JNI ERROR:fail to JVM::GetEnv");
	}
}
void setJVM(JavaVM * jvm){
	static std::once_flag oc;
	assert(nullptr!=jvm);
	std::call_once(oc, [jvm] {  JVM=jvm;});
}
JNIEnv* getJNIEnv() {
	if (nullptr == env) {
		_getJNIEnv();
	}
	return env;
}
void throwByName(const char* name, const char* msg) {
	auto cls = raii_FindClass_LocalRef(name);
	/* if cls is NULL, an exception has already been thrown */
	if (cls.get() != nullptr) {
		getJNIEnv()->ThrowNew(cls.get(), msg);
	} else {
		throw std::invalid_argument(std::string("not found java class:").append(name).data());
	}
}

void throwIllegalArgumentException(const char* msg) {
	throwByName("java/lang/IllegalArgumentException", msg);
}

/* 抛出std::logic_error, 并向cerr输出 */
void throw_fault(const char* msg) {
	std::cerr << msg << std::endl;
	throw std::logic_error(msg);
}
JavaClassMirror::JavaClassMirror(std::string canonicalName, std::pair<std::string,std::string> constr, std::initializer_list<std::pair<std::string,std::string> > field_signature)
:javaclass(jni_utilits::raii_FindClass_GlobalRef(canonicalName.data())), constructor(jni_utilits::getJNIEnv()->GetMethodID(javaclass.get(), constr.first.data(), constr.second.data()))
{
	auto env = jni_utilits::getJNIEnv();
	for (auto node : field_signature) {
		auto f = env->GetFieldID(javaclass.get(), node.first.data(), node.second.data());
		assert(nullptr != f);
		field.emplace(node.first, f);
	}
}

/*
 * 从jbyteArray中复制字节数组到分配的内存中(调用者负责释放指针),
 * 如果limit>0,那么jbytes的长度必须与limit相等,否则抛出异常,
 * release_mode取值 0,JNI_COMMIT,JNI_ABORT参见《Java Native Interface Specification Contents》
 * http://docs.oracle.com/javase/8/docs/technotes/guides/jni/spec/functions.html#Release_PrimitiveType_ArrayElements_routines
 * 返回分配内存的指针
 * */
uint8_t * copyToBytePtr(jbyteArray jbytes, jsize limit,jint release_mode) {
	if(nullptr == jbytes){
		jni_throw_if_msg(limit>0,"invalid jbyteArray")
		return nullptr;
	}
	auto size = jni_utilits::getJNIEnv()->GetArrayLength(jbytes);
	jni_throw_if_msg(limit >=0 && size != limit, "invalid jbyteArray length")
	if (size>0) {
		auto raii_byte_ptr = jni_utilits::raii_GetByteArrayElements(jbytes, release_mode);
		if ( JNI_COMMIT == release_mode ) {
			// JNI_COMMIT模式下，raii_byte_ptr指针的内存不会被JVM释放,可以省去数据复制直接返回此指针
			return reinterpret_cast<uint8_t*>(*raii_byte_ptr);
		}			
		else {
			auto ptr = new uint8_t[size];
			memcpy(ptr, *raii_byte_ptr, size);
			return ptr;
		}		
	}
	else
		return nullptr;
}
/*
 * 从jbyteArray中复制字节数组到std::vector<uint8_t>中,
 * 如果limit>0,那么jbytes的长度必须与limit相等,否则抛出异常
 * 返回std::vector对象
 * */
std::vector<uint8_t>  copyToVector(jbyteArray jbytes,jsize limit) {
	if(nullptr == jbytes){
		jni_throw_if_msg(limit>0,"invalid jbyteArray")
		return std::vector<uint8_t>(0);
	}
	auto size = jni_utilits::getJNIEnv()->GetArrayLength(jbytes);
	assert(size >= 0);
	jni_throw_if_msg(limit>=0&&size != limit,"invalid jbyteArray length")
	if (size>0) {
		auto byte_ptr = jni_utilits::raii_GetByteArrayElements(jbytes);
		return std::vector<uint8_t>(*byte_ptr, *byte_ptr + size);
	}
	else {
		return std::vector<uint8_t>(0);
	}
}
/*
 *  判断jobj是否是clazz的实例
 * clazz为nullptr时抛出异常
 * */
bool jclass_isInstance(jclass clazz,jobject jobj){
	jni_throw_if(nullptr==clazz)
	auto env = getJNIEnv();
	auto method_isInstance=env->GetMethodID(*jni_utilits::raii_GetObjectClass(clazz),"isInstance","(Ljava/lang/Object;)Z");
	return env->CallBooleanMethod(clazz,method_isInstance,jobj)==JNI_TRUE;
}
/*
 *  判断clazz是否数组
 * clazz为nullptr时抛出异常
 * */
bool jclass_isArray(jclass clazz){
	jni_throw_if(nullptr==clazz)
	auto env = getJNIEnv();
	auto method_isArray=env->GetMethodID(*jni_utilits::raii_GetObjectClass(clazz),"isArray","()Z");
	return env->CallBooleanMethod(clazz,method_isArray)==JNI_TRUE;
}
/*
 *  调用 java.lang.Class.getComponentType返回数组的元素类型
 *  clazz为nullptr时抛出异常
 * */

raii_var<jclass> jclass_getComponentType(jclass clazz){
	return jni_utilits::raii_CallObjectMethod<jclass>(clazz,"getComponentType","()Ljava/lang/Class;");
}
/*
 *  调用java.lang.Class.getName方法返回java类名(jobject)<br>
 * clazz为nullptr时抛出异常
 * */
raii_var<jstring> jclass_getNameJString(jclass clazz){
	jni_throw_if(nullptr==clazz)
	return jni_utilits::raii_CallObjectMethod<jstring>(clazz,"getName","()Ljava/lang/String;");
}
/*
 *  返回java类名(std::string对象)<br>
 * clazz为nullptr时抛出异常
 * */
std::string jclass_getName(jclass clazz){
	auto jstr_classname=jclass_getNameJString(clazz);
	auto raii_jbyte_name=jstring_getBytes(*jstr_classname);
	auto name_ptr=raii_GetByteArrayElements(*raii_jbyte_name,0);
	auto len=jstring_length(*jstr_classname);
	return std::string((char*)name_ptr.get(),size_t(len));
}
/*
 *  调用 java.lang.String.getBytes方法返回(java.lang.String)字符串的字节数组
 * clazz为nullptr时抛出异常
 * */
raii_var<jbyteArray> jstring_getBytes(jstring jstr){
	jni_throw_if(nullptr==jstr)
	return raii_CallObjectMethod<jbyteArray>(jstr,"getBytes","()[B");
}
/*
 *  调用 java.lang.String.length方法返回(java.lang.String)字符串长度
 * clazz为nullptr时抛出异常
 * */
jint jstring_length(jstring jstr){
	jni_throw_if(nullptr==jstr)
	auto env = getJNIEnv();
	auto jclass_string=raii_GetObjectClass(jstr);
	auto method_length=env->GetMethodID(*jclass_string,"length","()I");
	return env->CallIntMethod(jstr,method_length);
}

}/* namespace jni_utilits */
}/* namespace gdface */


