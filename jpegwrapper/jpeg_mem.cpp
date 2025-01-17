/*
 * jpeg_mem.cpp
 *
 *  Created on: 2016年1月19日
 *      Author: guyadong
 */
#include <algorithm>
#include <type_traits>
#include <iostream>
#include <vector>
#include <cstring>
#include "jpeg_mem_advance.h"
#include "../bin/dependent_sources/assert_macros.h"
#include "../bin/dependent_sources/raii.h"
jpeg_custom_output_fun jpeg_custom_output_default=[](j_common_ptr){};
/* 自定义jpeg图像压缩/解压缩过程中错误退出函数 */
METHODDEF(void) jpeg_mem_error_exit (j_common_ptr cinfo) {
	// 调用 format_message 生成错误信息
	char err_msg[JMSG_LENGTH_MAX];
	(*cinfo->err->format_message) (cinfo,err_msg);
	// 抛出c++异常
	throw jpeg_mem_exception(err_msg);
}
/* 将图像数据输出为jpeg格式的内存数据块,调用传入的finishe_output回调函数来处理压缩后的内存图像数据
 * 图像信息及获取图像行数据的方式都由compress_instance定义
 * custom用于设置图像输出参数
 * 出错抛出 jpeg_mem_exception
 */
void save_jpeg_mem(jpeg_compress_interface& compress_instance,
									const mem_finish_output_fun& finish_output,
									const unsigned int quality
									){
	// 输出图像数据缓冲区
	unsigned char* outBuffer = nullptr;
	// 输出图像数据缓冲区长度(压缩后的图像大小)
	unsigned long bufferSize = 0;
	bool jpeg_compress_finished=false;
	// 定义一个压缩对象
	jpeg_compress_struct cinfo;
	//用于错误信息
	jpeg_error_mgr jerr;
	// 错误输出绑定到压缩对象
	cinfo.err = jpeg_std_error(&jerr);
	// 设置自定义的错误处理函数
	jerr.error_exit = jpeg_mem_error_exit;
	// RAII对象在函数结束时释放资源
	gdface::raii buffer_guard([&](){
		jpeg_finish_compress(&cinfo);
		jpeg_destroy_compress(&cinfo);
		// 压缩正常结束则调用finish_output
		if(jpeg_compress_finished)
			finish_output(outBuffer, bufferSize);
		if (nullptr != outBuffer)
			free(outBuffer);
	});
	// 初始化压缩对象
	jpeg_create_compress(&cinfo);
	jpeg_mem_dest(&cinfo, &outBuffer, &bufferSize); // 设置内存输出缓冲区
	compress_instance.start_output(cinfo);
	jpeg_set_defaults(&cinfo);
	jpeg_set_quality(&cinfo, quality<100?quality:100, true);
	compress_instance.custom_output((j_common_ptr)(&cinfo)); // 执行自定义参数设置函数
	jpeg_start_compress(&cinfo, true);
	while (cinfo.next_scanline < cinfo.image_height) {
		compress_instance.get_pixel_rows(cinfo.next_scanline);
		jpeg_write_scanlines(&cinfo, compress_instance.buffer.data(), (JDIMENSION)compress_instance.buffer.size());
	}
	jpeg_compress_finished=true;
}

void save_jpeg_mem(const fs_image_matrix &matrix,
									const mem_finish_output_fun& finish_output,
									const unsigned int quality,
									const jpeg_custom_output_fun &custom
									){
	jpeg_compress_default default_compress_instance(matrix);
	default_compress_instance.custom_output= custom;
	save_jpeg_mem(default_compress_instance,	finish_output);
}
std::string jwp_save_jpeg_mem_as_string(const fs_image_matrix & matrix, const unsigned int quality) {
	std::string out;
	save_jpeg_mem(matrix,
		[&](const uint8_t *img, size_t size) {
		out = std::string((char*)img, size);
	}, quality);
	return out;
}
std::vector<uint8_t> jwp_save_jpeg_mem_as_vector(const fs_image_matrix & matrix, const unsigned int quality) {
	std::vector<uint8_t> out;
	save_jpeg_mem(matrix,
		[&](const uint8_t *img, size_t size) {
		out = std::vector<uint8_t>(img, img + size);
	}, quality);
	return out;
}
void save_jpeg_gray_mem(const fs_image_matrix &matrix,
									const mem_finish_output_fun& finish_output,
									const unsigned int quality
									){
	static auto custom_output_gray=[](j_common_ptr cinfo) {
			jpeg_set_colorspace((j_compress_ptr)cinfo, JCS_GRAYSCALE);
		};
	save_jpeg_mem(matrix,finish_output,quality,custom_output_gray);
}
std::string jwp_save_jpeg_gray_mem_as_string(const fs_image_matrix & matrix, const unsigned int quality) {
	std::string out;
	save_jpeg_gray_mem(matrix,
		[&](const uint8_t *img, size_t size) {
		out = std::string((char*)img, size);
	}, quality);
	return out;
}
std::vector<uint8_t> jwp_save_jpeg_gray_mem_as_vector(const fs_image_matrix & matrix, const unsigned int quality) {
	std::vector<uint8_t> out;
	save_jpeg_gray_mem(matrix,
		[&](const uint8_t *img, size_t size) {
		out = std::vector<uint8_t>(img, img + size);
	}, quality);
	return out;
}
/* 将jpeg格式的内存数据块jpeg_data解压缩 
 * 图像行数据存储的方式都由decompress_instance定义
 * 如果数据为空或读取数据出错抛出 jpeg_mem_exception
 */
void load_jpeg_mem(const uint8_t *jpeg_data,size_t size,
		 jpeg_decompress_interface &decompress_instance) {
	if(nullptr==jpeg_data||0==size)
		throw jpeg_mem_exception("empty image data");
	// 定义一个压缩对象 
	jpeg_decompress_struct  dinfo;
	//用于错误信息 
	jpeg_error_mgr jerr;
	// 错误输出绑定到压缩对象 
	dinfo.err = jpeg_std_error(&jerr);
	// 设置自定义的错误处理函数 
	jerr.error_exit = jpeg_mem_error_exit;
	// RAII对象在函数结束时释放资源
	gdface::raii buffer_guard([&](){
		jpeg_finish_decompress(&dinfo);
		jpeg_destroy_decompress(&dinfo);
	});
	// 初始化压缩对象
	jpeg_create_decompress(&dinfo);
	jpeg_mem_src(&dinfo, (uint8_t*)jpeg_data, (unsigned long)size); // 设置内存输出缓冲区
	(void) jpeg_read_header(&dinfo, true);
	decompress_instance.custom_output((j_common_ptr)&dinfo); // 执行自定义参数设置函数
	(void) jpeg_start_decompress(&dinfo);
	// 输出通道数必须是1/3/4
	if (dinfo.output_components != 1 && dinfo.output_components != 3 && dinfo.output_components != 4) {
		throw jpeg_mem_exception(
			"load_jpeg_mem(): Failed to load JPEG data cause by output_components error");
	}
	decompress_instance.start_output(dinfo);
	JDIMENSION num_scanlines;
	JDIMENSION expectd_lines;
	while (dinfo.output_scanline  < dinfo.output_height) {
		num_scanlines = jpeg_read_scanlines(&dinfo, decompress_instance.buffer.data(),
				(JDIMENSION)decompress_instance.buffer.size());
		expectd_lines=std::min((dinfo.output_height-dinfo.output_scanline),(JDIMENSION)decompress_instance.buffer.size());
		// 如果取到的行数小于预期的行数，则图像数据不完整抛出异常
        if (num_scanlines<expectd_lines)
        	throw jpeg_mem_exception("load_jpeg_mem(): Incomplete data");
		decompress_instance.put_pixel_rows(num_scanlines);
	}
}

fs_image_matrix load_jpeg_mem(const uint8_t *jpeg_data,size_t size,	const jpeg_custom_output_fun &custom) {
	jpeg_decompress_default default_decompress_instance;
	default_decompress_instance.custom_output = custom;
	load_jpeg_mem(jpeg_data,size,default_decompress_instance);
	return std::move(default_decompress_instance.matrix);
}

fs_image_matrix jwp_load_jpeg_mem(const void * jpeg_data, size_t size)
{
	return load_jpeg_mem((uint8_t *)jpeg_data, size);
}

fs_image_matrix jwp_load_jpeg_gray_mem(const void *jpeg_data,size_t size) {
	static auto custom_output_gray=[](j_common_ptr dinfo) {
			((j_decompress_ptr)dinfo)->out_color_space = JCS_GRAYSCALE;
		};
	return load_jpeg_mem((uint8_t *)jpeg_data,size,custom_output_gray);
}
/* 图像数据(输出/输入)接口 */
struct jpeg_io_interface{
	// 虚函数用于初始化图像数据源或目标
	virtual void open(j_common_ptr info)const=0;
	// 虚函数用于关闭数据IO
	virtual void close()const =0;
	virtual ~jpeg_io_interface()=default;
};
template<bool COMPRESS>
struct jpeg_io_mem:public jpeg_io_interface{
	mutable uint8_t *jpeg_data=nullptr;
	mutable unsigned long size=0;
	template<bool C=COMPRESS,typename Enable=typename std::enable_if<!C>::type>
	jpeg_io_mem(const uint8_t *jpeg_data,size_t size):jpeg_data(const_cast<uint8_t *>(jpeg_data)),size((unsigned long)size){}
	template<bool C=COMPRESS,typename Enable=typename std::enable_if<!C>::type>
	jpeg_io_mem(const std::vector<uint8_t> &jpeg_data):jpeg_data(const_cast<uint8_t *>(jpeg_data.data())),size((unsigned long)jpeg_data.size()){}
	virtual void open(j_common_ptr info)const {
		if (COMPRESS)
			jpeg_mem_dest((j_compress_ptr)info,&jpeg_data,&size);
		else {
			if (nullptr == jpeg_data || 0 == size)
				throw jpeg_mem_exception("empty image data");
			jpeg_mem_src((j_decompress_ptr)info, jpeg_data, size);
		}
	}
	virtual void close()const{}
};
template<bool COMPRESS>
struct jpeg_io_file:public jpeg_io_interface{
	jpeg_io_file( const char *const filename):jpeg_io_file(nullptr,filename){}
	jpeg_io_file(std::FILE *const file):jpeg_io_file(file,nullptr){}
	virtual void open(j_common_ptr info)const {
		if(COMPRESS)
			jpeg_stdio_dest((j_compress_ptr)info, file);
		else
			jpeg_stdio_src((j_decompress_ptr)info, file);
	}
	virtual void close()const{
		if(nullptr!=filename){
			fclose(file);
		}
	}
private:
	const char *const filename;
	mutable std::FILE * file;
	jpeg_io_file( std::FILE *const file,const char *const filename):file(file),filename(filename){
				if((nullptr==filename||0==strlen(filename))&&nullptr==file)
					throw std::invalid_argument("the argument filename and file must not all be nullptr (empty)");
				if(nullptr==this->file){
					if(nullptr==filename||0==strlen(filename))
						throw std::invalid_argument("the argument filename and file must not all be nullptr (empty)");
					if(nullptr==(this->file=fopen(filename,COMPRESS?"wb":"rb"))){
						throw std::invalid_argument(std::string("can't open file ").append(filename));
					}
				}
			}
};
/* (不解压缩)读取jpeg格式的内存数据块的基本信息返回image_matrix_pram对象
 * 如果数据为空或读取数据出错抛出 jpeg_mem_exception
 */
fs_image_matrix read_jpeg_header(const jpeg_io_interface &src) {
	// 定义一个压缩对象
	jpeg_decompress_struct  dinfo;
	// 用于错误信息
	jpeg_error_mgr jerr;
	// 错误输出绑定到压缩对象
	dinfo.err = jpeg_std_error(&jerr);
	// 设置自定义的错误处理函数
	jerr.error_exit = jpeg_mem_error_exit;
	// RAII对象在函数结束时释放资源
	gdface::raii buffer_guard([&](){
		src.close();
		jpeg_destroy_decompress(&dinfo);
	});
	// 初始化压缩对象
	jpeg_create_decompress(&dinfo);
	src.open((j_common_ptr)&dinfo);
	auto retcode = jpeg_read_header(&dinfo, true);
	throw_except_if_msg(jpeg_mem_exception, JPEG_HEADER_OK != retcode, "invalid jpeg header")
	fs_image_matrix matrix;
	// 填充图像基本信息结构
	matrix.width=dinfo.image_width;
	matrix.height=dinfo.image_height;
	matrix.color_space=(FS_COLOR_SPACE)dinfo.jpeg_color_space;
	matrix.channels=dinfo.num_components;
	//std::cout<<matrix.width<<"x"<<matrix.height<<"x"<<(uint32_t)matrix.channels<<" color="<<matrix.color_space<<std::endl;
	return std::move(matrix);
}
fs_image_matrix jwp_read_jpeg_header_file(const char *const filename) {
	return read_jpeg_header(jpeg_io_file<false>(filename));
}
fs_image_matrix jwp_read_jpeg_header_file(std::FILE *const file) {
	return read_jpeg_header(jpeg_io_file<false>(file));
}
fs_image_matrix jwp_read_jpeg_header_mem(const void *jpeg_data,size_t size) {
	return read_jpeg_header(jpeg_io_mem<false>((uint8_t*)jpeg_data,size));
}

jpeg_compress_default::jpeg_compress_default(const fs_image_matrix & matrix) :matrix(matrix), next_line(0) {
	row_stride = matrix.get_row_stride();
}

void jpeg_compress_default::start_output(jpeg_compress_struct & cinfo) {
	cinfo.image_width = matrix.width;
	cinfo.image_height = matrix.height;
	cinfo.input_components = matrix.channels;
	cinfo.in_color_space = (J_COLOR_SPACE)matrix.color_space;
	// buffer只保存一行像素的源数据指针
	buffer = std::vector<JSAMPROW>(1);
	next_line = 0;
}

void jpeg_compress_default::get_pixel_rows(JDIMENSION num_scanlines) {
	// buffer指向当前行像素数据的地址
	buffer[0] = const_cast<JSAMPROW>(matrix.pixels) + (next_line++)*row_stride*matrix.channels;
}

jpeg_decompress_default::jpeg_decompress_default(uint8_t align) :next_line(0), row_stride(0) {
	matrix.align = align;
}

jpeg_decompress_default::jpeg_decompress_default() : jpeg_decompress_default(0) {}

void jpeg_decompress_default::start_output(const jpeg_decompress_struct & dinfo) {
	// 填充图像基本信息结构
	auto b = fs_make_matrix(&matrix,
		dinfo.output_width,
		dinfo.output_height,
		dinfo.output_components,
		dinfo.data_precision,
		0,
		0, //????
		(FS_COLOR_SPACE)dinfo.out_color_space,
		0,
		nullptr);
	if (!b) {
		throw jpeg_mem_exception("fail to fs_make_matrix");
	}
	row_stride = matrix.get_row_stride();

	// buffer只保存一行像素的目标数据指针
	buffer = std::vector<JSAMPROW>(1);
	next_line = 0;
	// 初始化buffer指向第一像素存储地址
	buffer[next_line] = matrix.pixels;
}

void jpeg_decompress_default::put_pixel_rows(JDIMENSION num_scanlines) {
	// buffer指向下一行要像素存储地址
	buffer[0] = matrix.pixels + (++next_line)*row_stride*matrix.channels;
}
