#include "BufferManage.h"


/*
	��������������������еĴ��ļ�
	��Ա�������ͣ�
		m_File: ϵͳ�����ļ��Ĺ������� ���±�Ϊ���ص��ļ����
	���ܽ��ͣ������ڴ����д��ļ�
*/
class OpenFileTable {
	File m_File[100];
	InodeTable inode_table;
	BufferManager buffermanager;
public:
	/*
		��������: ��ϵͳ���ļ����з���һ�����е�File �ṹ
		�������ͣ�
			fd: ������ļ����
		����ֵ���ͣ�һ����е�File�ṹ
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
		cout << "�ѳ�������ļ�������" << endl;
		return nullptr;
	}

	/*
		��������: ��ϵͳ���ļ����л���File �ṹ
		��������:
			pfile: Ҫ���յ�file�ṹ
	*/
	void FClose(File *pfile)
	{
		pfile->set_count(pfile->get_count() - 1);
		if (pfile->get_count() < 0) {
			cout << "������뷢������" << endl;
			return;
		}
		if (pfile->get_count() == 0) {
			/* ����ǰFile�ṹ�м���Ϊ0�� ��ͬʱ����ָ���Inode���ü������� */
			/* �����ü�����Ϊ0�� ����ʹ����������DELWRI��ʶ�Ļ��������д�� */
			if (pfile->get_point_inode()->get_count() == 1) {
				buffermanager.WriteRelseBuffer(pfile->get_point_inode()->get_index(), &inode_table);
			}
			inode_table.InodeReduce(pfile->get_point_inode());
			pfile->set_point_inode(nullptr);
		}
	}

	/*
		��������: �����ļ���������ض�Ӧ��File�ṹ
		�������ͣ�
			fd��������ļ����
		����ֵ���ͣ���Ӧ��file�ṹ
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
