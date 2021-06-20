#define _CRT_SECURE_NO_WARNINGS
#include "StoreSegment.h"

const int OFFSET = 200;
/*
	类声明：文件存储管理类
	成员变量解释：
		superblock: 文件超级块的实例化
		diskinode: 外存inode的实例化
		datadisk: 空闲数据盘块的实例化
*/
class FileStoreManage {
	SuperBlock superblock;
	DiskInode diskinode[6576];

public:
	/*
		函数名字: 从文件中读取当前存在的SuperBlock和Inode情况
	*/
	FileStoreManage()
	{
		superblock.Load();
		for (int i = 0; i < sizeof(diskinode) / sizeof(DiskInode); i++)
			diskinode[i].Load(i);
	}

	/*
		函数名字: 完成文件存储管理的格式化操作
		功能解释：形成superblock的两个链表
	*/
	void format()
	{
		memset(this, 0, sizeof(FileStoreManage));
		for (int i = 0; i < sizeof(diskinode) / sizeof(DiskInode); i++)
			FileStoreInodeFree(i);
		for (int i = 0; i < sizeof(datadisk) / sizeof(DataDisk); i++)
			FileStoreDiskFree(i);
	}

	/*
		函数名字: 从superblock的s_inode数组中分配一个空闲的索引节点并返回
		返回值解释：分配的空闲外存索引节点
	*/
	int FileStoreInodeAlloc()
	{
		bool need_fill;
		int tmp = superblock.inode_alloc(need_fill);
		diskinode[tmp].set_dmode_alloc(true);
		if (need_fill) {
			int cnt = 0;
			for (int i = 0; i < sizeof(diskinode) / sizeof(DiskInode); i++) {
				bool is_alloc;
				diskinode[i].get_dmode_alloc(&is_alloc);
				if (!is_alloc) {
					superblock.inode_free(i);
					cnt++;
				}
				if (cnt == max_node)
					break;
			}
		}
		/* 若分配完成后s_ninode等于0，则表示所有空闲节点已经分配完毕 */
		//assert(superblock.get_sninode() > 0);

		memset(&diskinode[tmp], 0, sizeof(DiskInode));
		return tmp;
	}

	/*
		函数名字: 回收一个索引节点并将其放入s_inode数组中
		参数解释：
			index: 所需回收的外存索引节点下标
	*/
	void FileStoreInodeFree(const int index)
	{
		diskinode[index].set_dmode_alloc(false);
		superblock.inode_free(index);
	}

	/*
		函数名字: 从superblock的s_free数组中分配一个空闲的数据盘块并返回
		返回值解释：分配的空闲数据盘块
	*/
	int FileStoreDiskAlloc()
	{
		bool need_fill;
		int tmp = superblock.disk_alloc(need_fill);
		if (need_fill) {
			int buffer = 0;
			/* 使用s_free[0]去判断下一个成组链接是否存在 */
			memcpy(&buffer, tmp, sizeof(int), sizeof(int));
			/* 若该s_free[0] 为0， 则所有空闲数据块已经分配完毕 */
			assert(buffer != 0);

			/* 将下一个成组链接的值重新填充s_free和s_nfree*/
			buffer = 0;
			memcpy(&buffer, tmp, 0, sizeof(int));
			superblock.set_snfree(buffer);

			for (int i = 0; i < superblock.get_snfree(); i++) {
				buffer = 0;
				memcpy(&buffer, tmp, (i + 1) * sizeof(int), sizeof(int));
				superblock.set_sfree(i, buffer);
			}
		}
		char buf[512];
		memset(buf, 0, sizeof(buf));
		memcpy(tmp, buf, 0, BLOCK_SIZE);
		return tmp;
	}

	/*
		函数名字: 回收一个空闲数据盘块并将其放入s_free数组中
		参数解释：
			index: 所需回收的空闲数据盘块下标
	*/
	void FileStoreDiskFree(const int index)
	{
		bool need_fill;
		superblock.disk_free(index, need_fill);
		if (need_fill) {
			/* 向index数据盘块的前404字节填入当前superblock的s_nfree 和 s_free*/
			int buffer = superblock.get_snfree();
			memcpy(index, &buffer, 0, sizeof(int));

			for (int i = 0; i < 100; i++) {
				buffer = superblock.get_sfree(i);
				memcpy(index, &buffer, (i + 1) * sizeof(int), sizeof(int));
			}

			/* 更新s_nfree 和 s_free*/
			superblock.set_snfree(1);
			superblock.set_sfree(0, index);
		}
	}

	/*
		函数名字: 返回一个diskinode节点，给内存inode表提供接口
		参数解释：
			i_number: 所需获取的第i_number个diskinode节点
	*/
	DiskInode * FileStoreGetDiskInode(const int i_number)
	{
		return &diskinode[i_number];
	}

	/*
		函数名字: 更新一个diskinode节点，给内存inode表提供接口
		参数解释：
			inode: 用inode更新diskinode节点
	*/
	void FileStoreSetDiskInode(Inode &inode)
	{
		int i_number = inode.get_number();
		diskinode[i_number].set_dnlink(inode.get_nlink());
		diskinode[i_number].set_dsize(inode.get_size());
		for (int i = 0; i < 10; i++)
			diskinode[i_number][i] = inode[i];
		if (inode[6])
			diskinode[i_number].set_dmode_file_size(true);
	}

	/*
	函数名字: 用于在目录文件的diskinode中登记普通文件或目录文件
	参数解释：
		dir_index: 要登记的目录文件所对应的diskinode序号
		reg_name: 所需登记的文件名
		diskinode_index: 所需登记的文件名对应的diskInode的序号
	*/
	void file_reg(const int dir_index, const string reg_name, const int diskinode_index)
	{
		for (int i = 0; i <= 5; i++) {
			if (diskinode[dir_index][i] != 0 && datadisk[diskinode[dir_index][i] - 1].file_register(reg_name, diskinode_index, diskinode[dir_index][i] - 1) == true)
				return;
			else if (diskinode[dir_index][i] == 0) {
				/* 需要申请一块新的空闲盘块，用于登记信息 */
				int free_block = FileStoreDiskAlloc();
				/* 将分配好的数据块的序号+1，登记到d_addr数组中 */
				diskinode[dir_index][i] = free_block + 1;
				datadisk[diskinode[dir_index][i] - 1].file_register(reg_name, diskinode_index, diskinode[dir_index][i] - 1);
				return;
			}
		}
		for (int i = 6; i <= 7; i++) {
			if (diskinode[dir_index][i] != 0) {
				int buffer = diskinode[dir_index][i] - 1;
				for (int j = 0; j < BLOCK_SIZE / sizeof(int); j++) {
					int block_index = 0;
					memcpy(&block_index, buffer, j * sizeof(int), sizeof(int));
					if (block_index != 0 && datadisk[block_index - 1].file_register(reg_name, diskinode_index, block_index - 1) == true)
						return;
					else if (block_index == 0) {
						int free_block = FileStoreDiskAlloc();
						free_block += 1;
						memcpy(buffer, &free_block, j * sizeof(int), sizeof(int));
						datadisk[free_block - 1].file_register(reg_name, diskinode_index, free_block - 1);
						return;
					}
				}
			}
			else {
				int free_block = FileStoreDiskAlloc();
				diskinode[dir_index][i] = free_block + 1;
				/* 再次执行循环 */
				i--;
			}
		}
		for (int i = 8; i <= 9; i++) {
			if (diskinode[dir_index][i] != 0) {
				int buffer1 = diskinode[dir_index][i] - 1;
				for (int j = 0; j < BLOCK_SIZE / sizeof(int); j++) {
					int buffer2 = 0;
					memcpy(&buffer2, buffer1, j * sizeof(int), sizeof(int));
					if (buffer2 != 0) {
						buffer2 -= 1;
						for (int k = 0; k < BLOCK_SIZE / sizeof(int); k++) {
							int block_index = 0;
							memcpy(&block_index, buffer2, k * sizeof(int), sizeof(int));
							if (block_index != 0 && datadisk[block_index - 1].file_register(reg_name, diskinode_index, block_index - 1) == true)
								return;
							else if (block_index == 0) {
								int free_block = FileStoreDiskAlloc();
								free_block += 1;
								// 该值应该是k
								memcpy(buffer2, &free_block, k * sizeof(int), sizeof(int));
								datadisk[free_block - 1].file_register(reg_name, diskinode_index, free_block - 1);
								return;
							}
						}
					}
					else {
						int free_block = FileStoreDiskAlloc();
						free_block += 1;
						memcpy(buffer1, &free_block, j * sizeof(int), sizeof(int));
						/* 再次执行循环 */
						j--;
					}
				}
			}
			else {
				int free_block = FileStoreDiskAlloc();
				diskinode[dir_index][i] = free_block + 1;
				/* 再次执行循环 */
				i--;
			}
		}
		return;
	}

	/*
	函数名字: 用于将某块外存Inode的所有空闲数据块回收 并且释放自身所在外存Inode节点
	参数解释：
		diskindex：所要回收空闲数据块的外存inode序号
	*/
	void FileStoreReleaseDiskInodeArray(const int diskindex)
	{
		DiskInode *tmp = FileStoreGetDiskInode(diskindex);
		FileStoreInodeFree(diskindex);
		/* 小型文件数据块释放 */
		for (int i = 0; i <= 5; i++) {
			/* 登记数组中的数据块时，采用序号+1的方式进行 */
			if ((*tmp)[i] != 0) {
				FileStoreDiskFree((*tmp)[i] - 1);
				(*tmp)[i] = 0;
			}
			else
				break;
		}
		/* 大型文件数据块释放*/
		for (int i = 6; i <= 7; i++) {
			if ((*tmp)[i] != 0) {
				int buffer = (*tmp)[i] - 1;
				for (int j = 0; j < BLOCK_SIZE / sizeof(int); j++) {
					int block_index = 0;
					memcpy(&block_index, buffer, j * sizeof(int), sizeof(int));
					if (block_index != 0)
						FileStoreDiskFree(block_index - 1);
					else
						break;
				}
				/* 释放索引盘块后 将一级索引盘块同步释放*/
				FileStoreDiskFree(buffer);
				(*tmp)[i] = 0;
			}
			else
				break;
		}

		/* 巨型文件数据块释放 */
		for (int i = 8; i <= 9; i++) {
			if ((*tmp)[i] != 0) {
				int buffer1 = (*tmp)[i] - 1;
				for (int j = 0; j < BLOCK_SIZE / sizeof(int); j++) {
					int buffer2 = 0;
					memcpy(&buffer2, buffer1, j * sizeof(int), sizeof(int));
					if (buffer2 != 0) {
						buffer2 -= 1;
						for (int k = 0; k < BLOCK_SIZE / sizeof(int); k++) {
							int block_index = 0;
							memcpy(&block_index, buffer2, k * sizeof(int), sizeof(int));
							if (block_index != 0)
								FileStoreDiskFree(block_index - 1);
							else
								break;
						}
						FileStoreDiskFree(buffer2);
					}
					else
						break;
				}
				FileStoreDiskFree(buffer1);
				(*tmp)[i] = 0;
			}
			else
				break;
		}
	}

	/*
	函数名字: 用于将某块为目录结构的DiskInode所存储的信息中，删除其中一条目录项
	参数解释：
		src_index: 目录文件所对应的DiskInode
		diskindex: 需要删除的目录项对应的索引diskindex
	功能解释：寻找到diskindex所对应的目录项和最后一条目录项，二者交换后，将最后一项置零即可
	*/
	void FileStoreClearDirectoryEntry(const int src_index, const int diskindex)
	{
		/* 首先获取最后一项目录项 */
		int last_node;
		string last_name;
		diskinode[src_index].get_last_dir_entry(last_node, last_name);

		/* 一定要先使用最后一条记录覆盖后 再删除最后一条记录 */
		diskinode[src_index].disk_change_dir_entry(last_node, last_name, diskindex);
		diskinode[src_index].disk_change_dir_entry(0, "", last_node);
	}

	void save()
	{
		superblock.save();
		for (int i = 0; i < sizeof(diskinode) / sizeof(DiskInode); i++)
			diskinode[i].Save(i);
	}
};