﻿#ifndef SRC_UTIL_FILE_H_
#define SRC_UTIL_FILE_H_

#include <cstdio>
#include <cstdlib>
#include <string>
#include "util.h"
#include <functional>

#if defined(__linux__)
#include <limits.h>
#endif

#if defined(_WIN32)
#ifndef PATH_MAX
#define PATH_MAX 1024
#endif // !PATH_MAX

struct dirent{
    long d_ino;              /* inode number*/
    off_t d_off;             /* offset to this dirent*/
    unsigned short d_reclen; /* length of this d_name*/
    unsigned char d_type;    /* the type of d_name*/
    char d_name[1];          /* file name (null-terminated)*/
};
typedef struct _dirdesc {
    int     dd_fd;      /** file descriptor associated with directory */
    long    dd_loc;     /** offset in current buffer */
    long    dd_size;    /** amount of data returned by getdirentries */
    char    *dd_buf;    /** data buffer */
    int     dd_len;     /** size of data buffer */
    long    dd_seek;    /** magic cookie returned by getdirentries */
    HANDLE handle;
    struct dirent *index;
} DIR;
# define __dirfd(dp)    ((dp)->dd_fd)

int mkdir(const char *path, int mode);
DIR *opendir(const char *);
int closedir(DIR *);
struct dirent *readdir(DIR *);

#endif // defined(_WIN32)

#if defined(_WIN32) || defined(_WIN64)
#define fseek64 _fseeki64
#define ftell64 _ftelli64
#else
#define fseek64 fseek
#define ftell64 ftell
#endif

namespace FFZKit {

class File {
public:
    //创建路径  [AUTO-TRANSLATED:419b36b7]
    //Create path
    static bool create_path(const std::string &file, unsigned int mod);

    //新建文件，目录文件夹自动生成  [AUTO-TRANSLATED:e605efe8]
    //Create a new file, and the directory folder will be generated automatically
    static FILE *create_file(const std::string &file, const std::string &mode);

    //判断是否为目录  [AUTO-TRANSLATED:639e15fa]
    //Determine if it is a directory
    static bool is_dir(const std::string &path);

    //判断是否是特殊目录（. or ..）  [AUTO-TRANSLATED:f61f7e33]
    //Determine if it is a special directory (. or ..)
    static bool is_special_dir(const std::string &path);

    //删除目录或文件  [AUTO-TRANSLATED:79bed783]
    //Delete a directory or file
    static int delete_file(const std::string &path, bool del_empty_dir = false, bool backtrace = true);

    //判断文件是否存在  [AUTO-TRANSLATED:edf3cf49]
    //Determine if a file exists
    static bool fileExist(const std::string &path);

    /**
     * 加载文件内容至string
     * @param path 加载的文件路径
     * @return 文件内容
     * Load file content to string
     * @param path The path of the file to load
     * @return The file content
     
     * [AUTO-TRANSLATED:c2f0e9fa]
     */
    static std::string loadFile(const std::string &path);

    /**
     * 保存内容至文件
     * @param data 文件内容
     * @param path 保存的文件路径
     * @return 是否保存成功
     * Save content to file
     * @param data The file content
     * @param path The path to save the file
     * @return Whether the save was successful
     
     * [AUTO-TRANSLATED:a919ad75]
     */
    static bool saveFile(const std::string &data, const std::string &path);

    /**
     * 获取父文件夹
     * @param path 路径
     * @return 文件夹
     * Get the parent folder
     * @param path The path
     * @return The folder
     
     * [AUTO-TRANSLATED:3a584db5]
     */
    static std::string parentDir(const std::string &path);

    /**
     * 替换"../"，获取绝对路径
     * @param path 相对路径，里面可能包含 "../"
     * @param current_path 当前目录
     * @param can_access_parent 能否访问父目录之外的目录
     * @return 替换"../"之后的路径
     * Replace "../" and get the absolute path
     * @param path The relative path, which may contain "../"
     * @param current_path The current directory
     * @param can_access_parent Whether it can access directories outside the parent directory
     * @return The path after replacing "../"

     */
    static std::string absolutePath(const std::string &path, const std::string &current_path, bool can_access_parent = false);

    /**
     * 遍历文件夹下的所有文件
     * @param path 文件夹路径
     * @param cb 回调对象 ，path为绝对路径，isDir为该路径是否为文件夹，返回true代表继续扫描，否则中断
     * @param enter_subdirectory 是否进入子目录扫描
     * @param show_hidden_file 是否显示隐藏的文件
     * Traverse all files under the folder
     * @param path Folder path
     * @param cb Callback object, path is the absolute path, isDir indicates whether the path is a folder, returns true to continue scanning, otherwise stops
     * @param enter_subdirectory Whether to enter subdirectory scanning
     * @param show_hidden_file Whether to display hidden files
     
     * [AUTO-TRANSLATED:e97ab081]
     */
    static void scanDir(const std::string &path, const std::function<bool(const std::string &path, bool isDir)> &cb,
                        bool enter_subdirectory = false, bool show_hidden_file = false);

    /**
     * 获取文件大小
     * @param fp 文件句柄
     * @param remain_size true:获取文件剩余未读数据大小，false:获取文件总大小
     * Get file size
     * @param fp File handle
     * @param remain_size true: Get the remaining unread data size of the file, false: Get the total file size
     
     * [AUTO-TRANSLATED:9abfdae9]
     */
    static uint64_t fileSize(FILE *fp, bool remain_size = false);

    /**
     * 获取文件大小
     * @param path 文件路径
     * @return 文件大小
     * @warning 调用者应确保文件存在
     * Get file size
     * @param path File path
     * @return File size
     * @warning The caller should ensure the file exists
     
     * [AUTO-TRANSLATED:6985b813]
     */
    static uint64_t fileSize(const std::string &path);

    /**
     * 尝试删除空文件夹
     * @param dir 文件夹路径
     * @param backtrace 是否回溯上层文件夹，上层文件夹为空也一并删除，以此类推
     * Attempt to delete an empty folder
     * @param dir Folder path
     * @param backtrace Whether to backtrack to the upper-level folder, if the upper-level folder is empty, it will also be deleted, and so on
     
     * [AUTO-TRANSLATED:a1780506]
     */
    static void deleteEmptyDir(const std::string &dir, bool backtrace = true);

private:
    File();
    ~File();
};

} /* namespace FFZKit */
#endif /* SRC_UTIL_FILE_H_ */
