#include "BufferManage.h"


/*
	类声明：管理程序中所有的打开文件
	成员变量解释：
		m_File: 系统最大打开文件的管理数组 其下标为返回的文件句柄
	功能解释：管理内存所有打开文件
*/
class OpenFileTable {
	File m_File[100];
	InodeTable inode_table;
	BufferManager buffermanager;
public:
	/*
		函数名字: 在系统打开文件表中分配一个空闲的File 结构
		参数解释：
			fd: 分配的文件句柄
		返回值解释：一块空闲的File结构
	*/
	File *FAlloc(int &fd)
	{
		fd = -1;
		for (int i = 0; i < sizeof(m_File) / sizeof(File); i++) {
			if (m_File[i].get_count() == 0) {
				memset(&m_File[i], 0, sizeof(File));
				m_File[i].set_count(1);
				fd = i;
				return &m_File[i];
			}
		}
		cout << "已超出最大文件打开数量" << endl;
		return nullptr;
	}

	/*
		函数名字: 在系统打开文件表中回收File 结构
		参数解释:
			pfile: 要回收的file结构
	*/
	void FClose(File *pfile)
	{
		pfile->set_count(pfile->get_count() - 1);
		if (pfile->get_count() < 0) {
			cout << "句柄输入发生错误" << endl;
			return;
		}
		if (pfile->get_count() == 0) {
			/* 若当前File结构中计数为0， 则同时将其指向的Inode引用计数减少 */
			/* 若引用计数减为0， 则迫使缓冲区中有DELWRI标识的缓冲块立刻写入 */
			if (pfile->get_point_inode()->get_count() == 1) {
				buffermanager.WriteRelseBuffer(pfile->get_point_inode()->get_index(), &inode_table);
			}
			inode_table.InodeReduce(pfile->get_point_inode());
			pfile->set_point_inode(nullptr);
		}
	}

	/*
		函数名字: 根据文件句柄，返回对应的File结构
		参数解释：
			fd：输入的文件句柄
		返回值解释：对应的file结构
	*/
	File *FGet(const int fd)
	{
		return &m_File[fd];
	}

	InodeTable *get_inode_table()
	{
		return &inode_table;
	}

	int get_max_file_cnt()
	{
		return sizeof(m_File) / sizeof(File);
	}

	BufferManager * get_buffermanager()
	{
		return &buffermanager;
	}
};
