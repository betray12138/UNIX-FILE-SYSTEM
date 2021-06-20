#define _CRT_SECURE_NO_WARNINGS
#include <iostream>
#include <assert.h>
#include <string.h>
#include <deque>
#include <vector>
#include <string>
#include <algorithm>
#include <stdio.h>
#include <fstream>
using namespace std;
const static int max_node = 100;
const int BLOCK_SIZE = 512;
const char FILE_NAME[] = "./myDisk.img";
const int DATADISK_OFFSET = 1024;
fstream fp(FILE_NAME, ios::in | ios::out | ios::binary);

const enum RIGHT {
	write = 7,
	read,
	large = 12,
	dir,
	alloc = 15
};

/*
	函数名字: 重载memcpy拷贝函数 完成对文件的读取
	参数解释：
		dst_buf：读出后放置的位置
		datadisk_index: 数据盘块的序号 要求从0开始
		bias: 数据盘块内偏移量
		length：读取长度
*/
void memcpy(void *dst_buf, int datadisk_index, int bias, unsigned int length)
{
	fp.seekg((datadisk_index + DATADISK_OFFSET) * BLOCK_SIZE + bias, ios::beg);
	fp.read((char *)dst_buf, length);
}

/*
	函数名字: 重载memcpy拷贝函数 完成对文件的写入
	参数解释：
		datadisk_index: 数据盘块的序号 要求从0开始
		dst_buf：读出后放置的位置
		bias: 数据盘块内偏移量
		length：读取长度
*/
void memcpy(int datadisk_index, void *src_buf, int bias, unsigned int length)
{
	fp.seekg((datadisk_index + DATADISK_OFFSET) * BLOCK_SIZE + bias, ios::beg);
	fp.write((char *)src_buf, length);
}

/*
	类声明：目录树的目录结构 每个目录结构对应32字节，4字节为对应的Inode编号
	成员变量解释：
		m_inode: 每条目录路径对应的inode编号
		m_name: 对应目录下的文件名字
*/
class DirectoryEntry {
	int m_inode;
	char m_name[28];

public:
	DirectoryEntry() {
		m_inode = -1;
		memset(m_name, 0, sizeof(m_name));
	}

	DirectoryEntry(const int inode, string name)
	{
		m_inode = inode;
		strcpy(m_name, name.c_str());
	}

	int get_inode()
	{
		return m_inode;
	}

	void set_inode(const int inode)
	{
		m_inode = inode;
	}

	string get_name()
	{
		string tmp(m_name);
		return tmp;
	}

	void set_name(string name)
	{
		strcpy(m_name, name.c_str());
	}
};


/*
	类声明：数据盘块
	成员变量解释：
	功能解释：每个数据盘块512字节
*/
class DataDisk {
public:
	/*
		函数名字: 用于当前数据块中对应的目录名匹配
		参数解释：
			dir_name: 本层所需匹配的目录名
			disk_index: 数据盘块序号
		返回值解释：若匹配不到则返回-1. 否则返回指向下一层目录DiskInode的序号
	*/
	int dir_match(const string dir_name, const int disk_index)
	{
		/* 对于目录文件 其开始的4字节为下一层目录对应DiskInode的序号，其后28字节为本层目录名*/
		for (int i = 0; i < BLOCK_SIZE / sizeof(DirectoryEntry); i++) {
			DirectoryEntry tmp;
			memcpy(&tmp, disk_index, i * sizeof(DirectoryEntry), sizeof(DirectoryEntry));
			if (tmp.get_name() == dir_name)
				return tmp.get_inode();
		}
		return -1;
	}

	/*
		函数名字: 在目录文件的数据块中登入文件或者目录信息，共32字节
		参数解释：
			reg_name: 所需登记的文件名
			diskinode_index: 其所对应的diskInode的序号
			disk_index: 数据盘块序号
		返回值解释：若登记成功则返回true 若登记失败则返回false
	*/
	bool file_register(const string reg_name, const int diskinode_index, const int disk_index)
	{
		DirectoryEntry reg_dir(diskinode_index, reg_name);

		for (int i = 0; i < BLOCK_SIZE / sizeof(DirectoryEntry); i++) {
			DirectoryEntry tmp;
			memcpy(&tmp, disk_index, i * sizeof(DirectoryEntry), sizeof(DirectoryEntry));
			if (tmp.get_name() == string("") && tmp.get_inode() == 0) {
				memcpy(disk_index, &reg_dir, i * sizeof(DirectoryEntry), sizeof(DirectoryEntry));
				memcpy(&tmp, disk_index, i * sizeof(DirectoryEntry), sizeof(DirectoryEntry));
				return true;
			}
		}
		return false;
	}

	/*
		函数名字: 输出当前数据块包含的全部目录信息
		参数解释：
			disk_index: 数据盘块序号
	*/
	void print_item(const int disk_index)
	{
		for (int i = 0; i < BLOCK_SIZE / sizeof(DirectoryEntry); i++) {
			DirectoryEntry tmp;
			memcpy(&tmp, disk_index, i * sizeof(DirectoryEntry), sizeof(DirectoryEntry));
			if (tmp.get_name() == string("") && tmp.get_inode() == 0)
				return;
			else
				cout << tmp.get_name() << endl;
		}
	}

	/*
		函数名字: 返回数据块中最后一项目录记录的diskinode序号和name 并且将其清掉
		参数解释：
			inode: 使用引用的形式返回的最后一项记录的diskinode序号
			name: 使用引用的形式返回最后一项记录的名字
			disk_index: 数据盘块序号
			clear: 决定是否将最后一项记录清除
	*/
	void pop_dir_entry_last(int &inode, string &name, const int disk_index, bool clear = false)
	{
		inode = 0;
		name = "";
		for (int i = BLOCK_SIZE / sizeof(DirectoryEntry) - 1; i >= 0; i--) {
			DirectoryEntry tmp;
			memcpy(&tmp, disk_index, i * sizeof(DirectoryEntry), sizeof(DirectoryEntry));
			if (tmp.get_inode() == 0 && tmp.get_name() == string(""))
				continue;
			else if (tmp.get_inode() != 0 && tmp.get_name() != string("")) {
				inode = tmp.get_inode();
				name = tmp.get_name();
				/* 清除最后一项目录记录 */
				if (clear == true) {
					tmp.set_inode(0);
					tmp.set_name("");
					memcpy(disk_index, &tmp, i * sizeof(DirectoryEntry), sizeof(DirectoryEntry));
				}
				return;
			}
		}
	}

	/*
		函数名字: 在目录文件的数据块中更改目录项，使用inode1,name1 替换inode2
		参数解释：
			inode1: 更换的inode序号
			name1: 更换的name
			inode2：被更换的inode序号
			disk_index: 数据盘块序号
		返回值解释：若更换成功则返回true，更换失败则返回false
	*/
	bool change_dir_entry(const int inode1, const string name1, const int inode2, const int disk_index)
	{
		for (int i = 0; i < BLOCK_SIZE / sizeof(DirectoryEntry); i++) {
			DirectoryEntry tmp;
			memcpy(&tmp, disk_index, i * sizeof(DirectoryEntry), sizeof(DirectoryEntry));
			if (tmp.get_inode() == inode2 && tmp.get_name() != "") {
				tmp.set_name(name1);
				tmp.set_inode(inode1);
				memcpy(disk_index, &tmp, i * sizeof(DirectoryEntry), sizeof(DirectoryEntry));
				return true;
			}
		}
		return false;
	}
};

DataDisk datadisk[500000];

const int Disk_OFFSET = 202;
/*
	类声明：外存Inode节点
	成员变量解释：
		d_mode: 状态描述符:   7 写入权限 8 读取权限   12 文件是小型or大型巨型  13 文件是普通文件还是目录文件 15 该节点是否已分配
		d_nlink: 该文件在目录树中不同路径名的数量
		d_size: 文件大小，以字节为单位
		d_addr: 逻辑块号和物理盘块号的对应关系数组   0-5小型文件  6-7大型文件 8-9巨型文件
		padding: 用于占满64字节空间
	功能解释：每个Inode节点管理一个文件  每个DiskInode对应64字节
*/
class DiskInode
{
private:
	unsigned int d_mode;
	int d_nlink;
	int d_size;
	int d_addr[10];
	int padding[3];
public:
	void Load(const int index)
	{
		// index 从0开始
		fp.seekg(Disk_OFFSET * BLOCK_SIZE + index * sizeof(DiskInode), ios::beg);
		// 读取d_mode
		fp.read((char *)&d_mode, sizeof(int));
		// 读取d_nlink
		fp.read((char *)&d_nlink, sizeof(int));
		// 读取d_size
		fp.read((char *)&d_size, sizeof(int));
		// 读取d_addr数组
		for (int i = 0; i < sizeof(d_addr) / sizeof(int); i++) {
			fp.read((char *)&d_addr[i], sizeof(int));
		}
		// 读取padding数组
		for (int i = 0; i < sizeof(padding) / sizeof(int); i++) {
			fp.read((char *)&padding[i], sizeof(int));
		}
	}

	void Save(const int index)
	{
		// index 从0开始
		fp.seekg(Disk_OFFSET * BLOCK_SIZE + index * sizeof(DiskInode), ios::beg);
		// 写入d_mode
		fp.write((char *)&d_mode, sizeof(int));
		// 写入d_nlink
		fp.write((char *)&d_nlink, sizeof(int));
		// 写入d_size
		fp.write((char *)&d_size, sizeof(int));
		// 写入d_addr数组
		for (int i = 0; i < sizeof(d_addr) / sizeof(int); i++) {
			fp.write((char *)&d_addr[i], sizeof(int));
		}
		// 写入padding数组
		for (int i = 0; i < sizeof(padding) / sizeof(int); i++) {
			fp.write((char *)&padding[i], sizeof(int));
		}

	}

	/*
		函数名字: 从DiskInode的dmode中获取访问权限
		参数解释：
			is_w: 通过指针方式返回文件主是否具有对该文件写的权限
			is_r: 通过指针方式返回文件主是否具有对该文件读的权限
	*/
	void get_dmode_wr(bool *is_w, bool *is_r)
	{
		*is_w = d_mode & (1 << write);
		*is_r = d_mode & (1 << read);
	}

	/*
		函数名字: 为DiskInode的dmode设置读写权限
		参数解释：
			is_w: 设置是否具有对该文件写的权限
			is_r: 设置是否具有对该文件读的权限
	*/
	void set_dmode_wr(const bool is_w, const bool is_r)
	{
		d_mode = d_mode | (int(is_w) << write) | (int(is_r) << read);
	}

	/*
		函数名字: 从DiskInode的dmode中获取文件长度类型
		参数解释：
			is_large: 通过指针方式返回文件是否为大型或巨型文件
	*/
	void get_dmode_file_size(bool *is_large)
	{
		*is_large = d_mode & (1 << large);
	}

	/*
		函数名字: 为DiskInode的dmode设置文件长度类型
		参数解释：
			is_large: 设置文件是否为大型或巨型文件
	*/
	void set_dmode_file_size(const bool is_large)
	{
		d_mode = d_mode | (int(is_large) << large);
	}

	/*
		函数名字: 从DiskInode的dmode中获取文件是否为目录文件
		参数解释：
			is_dir: 通过指针方式返回文件是否为目录文件
	*/
	void get_dmode_file_mode(bool *is_dir)
	{
		*is_dir = d_mode & (1 << dir);
	}

	/*
		函数名字: 为DiskInode的dmode设置文件是否为目录文件
		参数解释：
			is_dir: 设置文件是否为目录文件
	*/
	void set_dmode_file_mode(const bool is_dir)
	{
		d_mode = d_mode | (int(is_dir) << dir);
	}

	/*
		函数名字: 从DiskInode的dmode中获取该节点是否已分配
		参数解释：
			is_alloc: 通过指针方式返回该节点是否已分配
	*/
	void get_dmode_alloc(bool *is_alloc)
	{
		*is_alloc = d_mode & (1 << alloc);
	}

	/*
		函数名字: 为DiskInode的dmode设置节点是否已分配
		参数解释：
			is_alloc: 设置文件是否已分配
	*/
	void set_dmode_alloc(const bool is_alloc)
	{
		d_mode = d_mode | (int(is_alloc) << alloc);
	}

	/*
		函数名字: 从DiskInode中获取文件在目录树中不同路径名的数量
	*/
	int get_dnlink()
	{
		return d_nlink;
	}

	/*
		函数名字: 为DiskInode设置不同路径名数量
		参数解释：
			nlink: 欲设置的不同路径名数量
	*/
	void set_dnlink(const int nlink)
	{
		d_nlink = nlink;
	}

	/*
		函数名字: 从DiskInode中获取文件大小
	*/
	int get_dsize()
	{
		return d_size;
	}

	/*
		函数名字: 为DiskInode设置文件大小
		参数解释：
			size: 欲设置的文件大小
	*/
	void set_dsize(const int size)
	{
		d_size = size;
	}

	/*
		函数名字: 重载[]运算符， 便于获取或修改d_addr中的值
		参数解释：
			index: d_addr中的数组下标
	*/
	int &operator [](const int index)
	{
		assert(index >= 0 && index < 10);
		return d_addr[index];
	}

	/*
		函数名字: 用于当前DiskInode中d_addr数组对应数据块的目录名匹配
		参数解释：
			dir_name: 本层所需匹配的目录名
		返回值解释：若匹配不到则返回-1. 否则返回指向下一层目录DiskInode的序号
	*/
	int dir_path_match(const string dir_name)
	{
		for (int i = 0; i <= 5; i++) {
			if (d_addr[i] != 0) {
				int res = datadisk[d_addr[i] - 1].dir_match(dir_name, d_addr[i] - 1);
				if (res != -1)
					return res;
			}
			else
				return -1;
		}
		for (int i = 6; i <= 7; i++) {
			if (d_addr[i] != 0) {
				int buffer = d_addr[i] - 1;
				for (int j = 0; j < BLOCK_SIZE / sizeof(int); j++) {
					int block_index = 0;
					memcpy(&block_index, buffer, j * sizeof(int), sizeof(int));
					if (block_index != 0) {
						int res = datadisk[block_index - 1].dir_match(dir_name, block_index - 1);
						if (res != -1)
							return res;
					}
					else
						return -1;
				}
			}
			else
				return -1;
		}
		for (int i = 8; i <= 9; i++) {
			if (d_addr[i] != 0) {
				int buffer1 = d_addr[i] - 1;
				for (int j = 0; j < BLOCK_SIZE / sizeof(int); j++) {
					int buffer2 = 0;
					memcpy(&buffer2, buffer1, j * sizeof(int), sizeof(int));
					if (buffer2 != 0) {
						buffer2 -= 1;
						for (int k = 0; k < BLOCK_SIZE / sizeof(int); k++) {
							int block_index = 0;
							memcpy(&block_index, buffer2, k * sizeof(int), sizeof(int));
							if (block_index != 0) {
								int res = datadisk[block_index - 1].dir_match(dir_name, block_index - 1);
								if (res != -1)
									return res;
							}
							else
								return -1;
						}
					}
					else
						return -1;
				}
			}
			else
				return -1;
		}
		return -1;
	}

	/*
		函数名字: 当前DiskInode为目录项，输出目录项包含的全部目录信息
	*/
	void print_dir_item()
	{
		for (int i = 0; i <= 5; i++) {
			if (d_addr[i] != 0) {
				datadisk[d_addr[i] - 1].print_item(d_addr[i] - 1);
			}
			else
				return;
		}
		for (int i = 6; i <= 7; i++) {
			if (d_addr[i] != 0) {
				int buffer = d_addr[i] - 1;
				for (int j = 0; j < BLOCK_SIZE / sizeof(int); j++) {
					int block_index = 0;
					memcpy(&block_index, buffer, j * sizeof(int), sizeof(int));
					if (block_index != 0)
						datadisk[block_index - 1].print_item(block_index - 1);
					else
						return;
				}
			}
			else
				return;
		}
		for (int i = 8; i <= 9; i++) {
			if (d_addr[i] != 0) {
				int buffer1 = d_addr[i] - 1;
				for (int j = 0; j < BLOCK_SIZE / sizeof(int); j++) {
					int buffer2 = 0;
					memcpy(&buffer2, buffer1, j * sizeof(int), sizeof(int));
					if (buffer2 != 0) {
						buffer2 -= 1;
						for (int k = 0; k < BLOCK_SIZE / sizeof(int); k++) {
							int block_index = 0;
							memcpy(&block_index, buffer2, k * sizeof(int), sizeof(int));
							if (block_index != 0)
								datadisk[block_index - 1].print_item(block_index - 1);
							else
								return;
						}
					}
					else
						return;
				}
			}
			else
				return;
		}
		return;
	}

	/*
		函数名字: 用于获取当前DiskInode中最后一项目录项
		参数解释：
			last_inode: 使用引用的形式返回的最后一项记录的diskinode序号
			last_name: 使用引用的形式返回最后一项记录的名字
	*/
	void get_last_dir_entry(int &last_inode, string &last_name)
	{
		int inode;
		string name;
		for (int i = 0; i <= 5; i++) {
			if (d_addr[i] != 0) {
				datadisk[d_addr[i] - 1].pop_dir_entry_last(inode, name, d_addr[i] - 1);
				if (inode != 0 && name != "") {
					last_inode = inode;
					last_name = name;
				}
				else
					break;
			}
			else
				break;
		}
		for (int i = 6; i <= 7; i++) {
			if (d_addr[i] != 0) {
				int buffer = d_addr[i] - 1;
				for (int j = 0; j < BLOCK_SIZE / sizeof(DirectoryEntry); j++) {
					int block_index = 0;
					memcpy(&block_index, buffer, j * sizeof(int), sizeof(int));
					if (block_index != 0) {
						datadisk[block_index - 1].pop_dir_entry_last(inode, name, block_index - 1);
						if (inode != 0 && name != "") {
							last_inode = inode;
							last_name = name;
						}
						else
							break;
					}
					else
						break;
				}
			}
			else
				break;
		}
		for (int i = 8; i <= 9; i++) {
			if (d_addr[i] != 0) {
				int buffer1 = d_addr[i] - 1;
				for (int j = 0; j < BLOCK_SIZE / sizeof(DirectoryEntry); j++) {
					int buffer2 = 0;
					memcpy(&buffer2, buffer1, j * sizeof(int), sizeof(int));
					if (buffer2 != 0) {
						buffer2 -= 1;
						for (int k = 0; k < BLOCK_SIZE / sizeof(DirectoryEntry); k++) {
							int block_index = 0;
							memcpy(&block_index, buffer2, k * sizeof(int), sizeof(int));
							if (block_index != 0) {
								datadisk[block_index - 1].pop_dir_entry_last(inode, name, block_index - 1);
								if (inode != 0 && name != "") {
									last_inode = inode;
									last_name = name;
								}
								else
									break;
							}
							else
								break;
						}
					}
					else
						break;
				}
			}
			else
				break;
		}
	}

	/*
		函数名字: 在当前diskinode对应的所有数据块中更改目录项，使用inode1,name1 替换inode2
		参数解释：
			inode1: 更换的inode序号
			name1: 更换的name
			inode2：被更换的inode序号
	*/
	void disk_change_dir_entry(const int inode1, const string name1, const int inode2)
	{
		for (int i = 0; i <= 5; i++) {
			if (d_addr[i] != 0) {
				if (datadisk[d_addr[i] - 1].change_dir_entry(inode1, name1, inode2, d_addr[i] - 1))
					return;
			}
			else
				return;
		}
		for (int i = 6; i <= 7; i++) {
			if (d_addr[i] != 0) {
				int buffer = d_addr[i] - 1;
				for (int j = 0; j < BLOCK_SIZE / sizeof(DirectoryEntry); j++) {
					int block_index = 0;
					memcpy(&block_index, buffer, j * sizeof(int), sizeof(int));
					if (block_index != 0) {
						if (datadisk[block_index - 1].change_dir_entry(inode1, name1, inode2, block_index - 1))
							return;
					}
					else
						return;
				}
			}
			else
				return;
		}
		for (int i = 8; i <= 9; i++) {
			if (d_addr[i] != 0) {
				int buffer1 = d_addr[i] - 1;
				for (int j = 0; j < BLOCK_SIZE / sizeof(DirectoryEntry); j++) {
					int buffer2 = 0;
					memcpy(&buffer2, buffer1, j * sizeof(int), sizeof(int));
					if (buffer2 != 0) {
						buffer2 -= 1;
						for (int k = 0; k < BLOCK_SIZE / sizeof(DirectoryEntry); k++) {
							int block_index = 0;
							memcpy(&block_index, buffer2, k * sizeof(int), sizeof(int));
							if (block_index != 0) {
								if (datadisk[block_index - 1].change_dir_entry(inode1, name1, inode2, block_index - 1))
									return;
							}
							else
								return;
						}
					}
					else
						return;
				}
			}
			else
				return;
		}
	}
};

const int SP_OFFSET = 200;
/*
	类声明：文件系统超级块
	成员变量解释：
		s_nfree: 直接管理的空闲盘块数量
		s_free: 直接管理的空闲盘块索引表
		s_ninode： 直接管理的空闲外存inode节点: 范围 1-100 初始为0
		s_inode: 直接管理的空闲外存inode索引表
		padding: 用于占满1024字节空间
*/
class SuperBlock {
	int s_nfree;
	int s_free[100];

	int s_ninode;
	int s_inode[100];

	int padding[54];

public:
	void Load()
	{
		fp.seekg(SP_OFFSET * BLOCK_SIZE, ios::beg);
		fp.read((char *)&s_nfree, sizeof(int));
		// 读取s_free数组
		for (int i = 0; i < sizeof(s_free) / sizeof(int); i++) {
			fp.read((char *)&s_free[i], sizeof(int));
		}
		// 读取s_ninode
		fp.read((char *)&s_ninode, sizeof(int));
		// 读取s_inode数组
		for (int i = 0; i < sizeof(s_inode) / sizeof(int); i++) {
			fp.read((char *)&s_inode[i], sizeof(int));
		}
		// 读取padding数组
		for (int i = 0; i < sizeof(padding) / sizeof(int); i++) {
			fp.read((char *)&padding[i], sizeof(int));
		}
	}

	void save()
	{
		fp.seekg(SP_OFFSET * BLOCK_SIZE, ios::beg);
		fp.write((char *)&s_nfree, sizeof(int));
		// 写入s_free数组
		for (int i = 0; i < sizeof(s_free) / sizeof(int); i++) {
			fp.write((char *)&s_free[i], sizeof(int));
		}
		// 写入s_ninode
		fp.write((char *)&s_ninode, sizeof(int));
		// 写入s_inode数组
		for (int i = 0; i < sizeof(s_inode) / sizeof(int); i++) {
			fp.write((char *)&s_inode[i], sizeof(int));
		}
		// 写入padding数组
		for (int i = 0; i < sizeof(padding) / sizeof(int); i++) {
			fp.write((char *)&padding[i], sizeof(int));
		}


	}

	/*
		函数名字: 从s_inode数组中分配一个空闲的索引节点并返回
		参数解释：
			need_fill: 当s_ninode变为0时，需要重新装填s_inode数组
		返回值解释：分配的空闲外存索引节点
	*/
	int inode_alloc(bool &need_fill)
	{
		need_fill = false;
		int tmp = s_inode[--s_ninode];
		if (s_ninode == 0)
			need_fill = true;
		return tmp;
	}

	/*
		函数名字: 回收一个索引节点并将其放入s_inode数组中
		参数解释：
			index: 所需回收的外存索引节点下标
	*/
	void inode_free(const int index)
	{
		if (s_ninode == max_node)
			return;
		s_inode[s_ninode++] = index;
	}

	/*
		函数名字: 从s_free数组中分配一个空闲的数据盘块并返回
		参数解释：
			need_fill: 当s_nfree变为0时，需要重新装填s_free数组
		返回值解释：分配的空闲数据盘块
	*/
	int disk_alloc(bool &need_fill)
	{
		need_fill = false;
		int tmp = s_free[--s_nfree];
		if (s_nfree == 0)
			need_fill = true;
		return tmp;
	}

	/*
		函数名字: 回收一个空闲盘块并存入s_free数组中
		参数解释：
			index: 所需回收的空闲盘块下标
			need_fill: 当s_nfree变为100时，需要重新装填s_free数组
	*/
	void disk_free(const int index, bool &need_fill)
	{
		need_fill = false;
		if (s_nfree == max_node) {
			need_fill = true;
			return;
		}
		s_free[s_nfree++] = index;
	}

	int get_snfree()
	{
		return s_nfree;
	}

	int get_sninode()
	{
		return s_ninode;
	}

	void set_snfree(const int nfree)
	{
		s_nfree = nfree;
	}

	void set_sninode(const int ninode)
	{
		s_ninode = ninode;
	}

	void set_sfree(const int index, const int value)
	{
		s_free[index] = value;
	}

	void set_sinode(const int index, const int value)
	{
		s_inode[index] = value;
	}

	int get_sfree(const int index)
	{
		return s_free[index];
	}

	int get_sinode(const int index)
	{
		return s_inode[index];
	}
};


/*
	类声明：内存Inode节点 每个Inode节点唯一对应外存DiskInode节点
	成员变量解释：
		i_mode: 状态描述符:   7 写入权限 8 读取权限
		i_update: 内存inode被修改过 需要更新相应外存inode
		i_count: 该索引节点当前活跃的实例数目，若值为0，则该节点空闲，释放并写回
		i_nlink: 该文件在目录树中不同路径名的数量
		i_size: 文件大小，单位为字节
		i_addr: 文件逻辑块号和物理块号转换索引表
		index: Inode节点的序号
	功能解释：每个Inode节点对应一个外存inode节点
*/
class Inode {
	unsigned int i_mode;
	int i_update;
	int i_count;
	int i_nlink;
	int i_number;
	int i_size;
	int i_addr[10];
	int index;

public:
	Inode() {
		i_mode = 0;
		i_count = 0;
		i_nlink = 0;
		i_number = 0;
		i_size = 0;
		memset(i_addr, 0, sizeof(i_addr));
	}

	void Set_index(const int i_index)
	{
		index = i_index;
	}

	/*
		函数名字: 从Inode的imode中获取访问权限
		参数解释：
			is_w: 通过指针方式返回文件主是否具有对该文件写的权限
			is_r: 通过指针方式返回文件主是否具有对该文件读的权限
	*/
	void get_imode_wr(bool *is_w, bool *is_r)
	{
		*is_w = i_mode & (1 << write);
		*is_r = i_mode & (1 << read);
	}

	/*
		函数名字: 为Inode的imode设置读写权限
		参数解释：
			is_w: 设置是否具有对该文件写的权限
			is_r: 设置是否具有对该文件读的权限
	*/
	void set_imode_wr(const bool is_w, const bool is_r)
	{
		i_mode = i_mode | (int(is_w) << write) | (int(is_r) << read);
	}

	/*
		函数名字: 从Inode的imode中获取是目录文件还是普通文件
		参数解释：
			is_dir: 通过指针方式返回文件是否是目录文件
	*/
	void get_imode_dir(bool *is_dir)
	{
		*is_dir = i_mode & (1 << dir);
	}

	/*
		函数名字: 为Inode的imode设置是否是目录文件
		参数解释：
			is_dir: 输入的是否是目录文件
	*/
	void set_imode_dir(const bool is_dir)
	{
		i_mode = i_mode | (int(is_dir) << dir);
	}

	int get_update()
	{
		return i_update;
	}

	void set_update(const int _update)
	{
		i_update = _update;
	}

	int get_count()
	{
		return i_count;
	}

	void set_count(const int cnt)
	{
		i_count = cnt;
	}

	int get_nlink()
	{
		return i_nlink;
	}

	void set_nlink(const int link)
	{
		i_nlink = link;
	}

	int get_number()
	{
		return i_number;
	}

	void set_number(const int number)
	{
		i_number = number;
	}

	int get_size()
	{
		return i_size;
	}

	void set_size(const int size)
	{
		i_size = size;
	}

	int get_index()
	{
		return index;
	}

	/*
		函数名字: 重载[]运算符， 便于获取或修改i_addr中的值
		参数解释：
			index: i_addr中的数组下标
	*/
	int &operator [](const int index)
	{
		assert(index >= 0 && index < 10);
		return i_addr[index];
	}

};


/*
	类声明：打开文件结构 用于描述用户可能以不同读写权限 打开同一文件 指向内存Inode
	成员变量解释：
		f_flag: 该打开文件结构对文件读写的要求   7 写权限  8 读权限
		f_count: 进程指向该结构的数量  对单一进程不存在分裂进程的情况 则最多只能为1
		f_inode: 指向内存Inode
		f_offset: 定义文件指针
*/
class File {
	unsigned int f_flag;
	int f_count;
	Inode *f_inode;
	int f_offset;

public:
	File()
	{
		f_flag = 0;
		f_count = 0;
		f_inode = nullptr;
		f_offset = 0;
	}

	/*
		函数名字: 从文件打开结构中获取权限类型
		参数解释：
			is_w: 通过指针方式返回文件主是否具有对该文件写的权限
			is_r: 通过指针方式返回文件主是否具有对该文件读的权限
	*/
	void get_dmode_wr(bool *is_w, bool *is_r)
	{
		*is_w = f_flag & (1 << write);
		*is_r = f_flag & (1 << read);
	}

	/*
		函数名字: 为Inode的imode设置读写权限
		参数解释：
			is_w: 设置是否具有对该文件写的权限
			is_r: 设置是否具有对该文件读的权限
	*/
	void set_imode_wr(const bool is_w, const bool is_r)
	{
		f_flag = f_flag | (int(is_w) << write) | (int(is_r) << read);
	}

	int get_count()
	{
		return f_count;
	}

	void set_count(const int cnt)
	{
		f_count = cnt;
	}

	Inode * get_point_inode()
	{
		return f_inode;
	}

	void set_point_inode(Inode *inode)
	{
		f_inode = inode;
	}

	int get_offset()
	{
		return f_offset;
	}

	void set_offset(const int offset)
	{
		f_offset = offset;
	}
};


/*
	类声明：控制跟踪一次读写的完成情况
	成员变量解释：
		m_offset: 当前读写文件的字节偏移量
		m_count: 当前还剩余的读写字节数量
*/
class IOParameter {
	int m_offset;
	int m_count;

public:
	IOParameter(const int offset, const int count):m_offset(offset), m_count(count)
	{
		return;
	}

	int get_offset()
	{
		return m_offset;
	}

	void set_offset(const int offset)
	{
		m_offset = offset;
	}

	int get_count()
	{
		return m_count;
	}

	void set_count(const int count)
	{
		m_count = count;
	}
};