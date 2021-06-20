#include "FileStoreManager.h"


/*
	类声明：内存Inode节点表
	成员变量解释：
		inode: 内存Inode节点表 管理所有内存打开文件
	功能解释：管理内存所有打开文件
*/
class InodeTable {
	Inode inode[100];
	FileStoreManage filestoremanage;
public:
	InodeTable()
	{
		for (int i = 0; i < sizeof(inode) / sizeof(Inode); i++) {
			inode[i].Set_index(i);
		}
	}
	/*
		函数名字: 根据传入的i_number获得指定DiskInode的拷贝存入Inode中并返回该Inode
		参数解释：
			i_number：指定的DiskInode序号
	*/
	Inode *InodeGetWithIndexOfDiskInode(const int i_number)
	{
		for (int i = 0; i < sizeof(inode) / sizeof(Inode); i++) {
			/* 判断存在指定的DiskInode号有两个标准，1是get_count大于0，2是i_number匹配*/
			if (inode[i].get_number() == i_number && inode[i].get_count() > 0) {
				inode[i].set_count(inode[i].get_count() + 1);
				return &inode[i];
			}
		}
		for (int i = 0; i < sizeof(inode) / sizeof(Inode); i++) {
			if (inode[i].get_count() == 0) {
				DiskInode * tmp = filestoremanage.FileStoreGetDiskInode(i_number);
				int tmp_index = inode[i].get_index();
				memset(&inode[i], 0, sizeof(Inode));
				inode[i].Set_index(tmp_index);
				bool is_w, is_r, is_dir;
				tmp->get_dmode_wr(&is_w, &is_r);
				tmp->get_dmode_file_mode(&is_dir);
				inode[i].set_imode_wr(is_w, is_r);
				inode[i].set_imode_dir(is_dir);
				inode[i].set_count(1);
				inode[i].set_nlink(tmp->get_dnlink());
				inode[i].set_number(i_number);
				inode[i].set_size(tmp->get_dsize());
				for (int j = 0; j < 10; j++)
					inode[i][j] = (*tmp)[j];
				return &inode[i];
			}
		}
		cout << "系统内存Inode数目已到达上限" << endl;
		return nullptr;
	}

	/*
		函数名字: 对传入的pNode 进行回收或者引用计数的减少
		参数解释：
			pNode: 传入的pNode
	*/
	void InodeReduce(Inode *pnode)
	{
		assert(pnode->get_count() > 0);
		/* 当系统中存在多个文件链接路径时，仅计数减少即可 */

		pnode->set_count(pnode->get_count() - 1);

		if (pnode->get_count() >= 1) {
			return;
		}

		if (pnode->get_nlink() == 0) {
			/* 当前文件没有勾连路径，则释放外存DiskInode 并且 释放其占用的数据盘块*/
			filestoremanage.FileStoreReleaseDiskInodeArray(pnode->get_number());
			return;
		}
		if (pnode->get_update()) {
			/* 若有更新标识 需要写回外存diskinode*/
			filestoremanage.FileStoreSetDiskInode(*pnode);
			/* 延迟写标识的处理 */
			pnode->set_update(0);
		}
	}

	FileStoreManage *get_filestoremanage()
	{
		return &filestoremanage;
	}

	Inode *get_Inode(const int index)
	{
		return &inode[index];
	}

	/*
	函数名字: 文件逻辑盘块到物理盘块的地址映射函数
	参数解释：
		inode_index: 内存Indoe节点所对应的序号
		lbn: 输入的文件逻辑盘块号
	返回值解释: 输出的逻辑盘块号所对应的物理盘块号, 若输出-1, 则表示文件过大
*/
	int Bmap(const int inode_index, const int lbn)
	{
		if (lbn >= 0 && lbn < 6) {
			if (inode[inode_index][lbn] == 0)
				inode[inode_index][lbn] = filestoremanage.FileStoreDiskAlloc() + 1;
			return inode[inode_index][lbn];
		}
		else if (lbn >= 6 && lbn < 128 * 2 + 6) {
			if (inode[inode_index][(lbn - 6) / (BLOCK_SIZE / sizeof(int)) + 6] == 0)
				inode[inode_index][(lbn - 6) / (BLOCK_SIZE / sizeof(int)) + 6] = filestoremanage.FileStoreDiskAlloc() + 1;
			
			int buffer = inode[inode_index][(lbn - 6) / (BLOCK_SIZE / sizeof(int)) + 6] - 1;
			int block_index = 0;
			memcpy(&block_index, buffer, (lbn - 6) % (BLOCK_SIZE / sizeof(int)) * sizeof(int), sizeof(int));
			if (block_index == 0) {
				block_index = filestoremanage.FileStoreDiskAlloc() + 1;
				memcpy(buffer, &block_index, (lbn - 6) % (BLOCK_SIZE / sizeof(int)) * sizeof(int), sizeof(int));
			}
			return block_index;
		}
		else if (lbn >= 128 * 2 + 6 && lbn < 128 * 128 * 2 + 128 * 2 + 6) {
			if (inode[inode_index][(lbn - 128 * 2 - 6) / (BLOCK_SIZE / sizeof(int)) / (BLOCK_SIZE / sizeof(int)) + 8] == 0)
				inode[inode_index][(lbn - 128 * 2 - 6) / (BLOCK_SIZE / sizeof(int)) / (BLOCK_SIZE / sizeof(int)) + 8] = filestoremanage.FileStoreDiskAlloc() + 1;
			int buffer1 = inode[inode_index][(lbn - 128 * 2 - 6) / (BLOCK_SIZE / sizeof(int)) / (BLOCK_SIZE / sizeof(int)) + 8] - 1;
			int buffer2 = 0;
			int buffer2_pos = (lbn >= 128 * 128 + 128 * 2 + 6 ? lbn - 128 * 128 : lbn);
			memcpy(&buffer2, buffer1, (buffer2_pos - 128 * 2 - 6) / (BLOCK_SIZE / sizeof(int)), sizeof(int));
			if (buffer2 == 0) {
				buffer2 = filestoremanage.FileStoreDiskAlloc() + 1;
				memcpy(buffer1, &buffer2, (buffer2_pos - 128 * 2 - 6) / (BLOCK_SIZE / sizeof(int)), sizeof(int));
			}
			buffer2 -= 1;
			int block_index = 0;
			memcpy(&block_index, buffer2, (lbn - 128 * 2 - 6) % (BLOCK_SIZE / sizeof(int)) * sizeof(int), sizeof(int));
			if (block_index == 0) {
				block_index = filestoremanage.FileStoreDiskAlloc() + 1;
				memcpy(buffer2, &block_index, (lbn - 128 * 2 - 6) % (BLOCK_SIZE / sizeof(int)) * sizeof(int), sizeof(int));
			}
			return block_index;
		}
		return -1;
	}
};
