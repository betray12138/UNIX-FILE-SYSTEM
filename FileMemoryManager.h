#include "FileStoreManager.h"


/*
	���������ڴ�Inode�ڵ��
	��Ա�������ͣ�
		inode: �ڴ�Inode�ڵ�� ���������ڴ���ļ�
	���ܽ��ͣ������ڴ����д��ļ�
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
		��������: ���ݴ����i_number���ָ��DiskInode�Ŀ�������Inode�в����ظ�Inode
		�������ͣ�
			i_number��ָ����DiskInode���
	*/
	Inode *InodeGetWithIndexOfDiskInode(const int i_number)
	{
		for (int i = 0; i < sizeof(inode) / sizeof(Inode); i++) {
			/* �жϴ���ָ����DiskInode����������׼��1��get_count����0��2��i_numberƥ��*/
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
		cout << "ϵͳ�ڴ�Inode��Ŀ�ѵ�������" << endl;
		return nullptr;
	}

	/*
		��������: �Դ����pNode ���л��ջ������ü����ļ���
		�������ͣ�
			pNode: �����pNode
	*/
	void InodeReduce(Inode *pnode)
	{
		assert(pnode->get_count() > 0);
		/* ��ϵͳ�д��ڶ���ļ�����·��ʱ�����������ټ��� */

		pnode->set_count(pnode->get_count() - 1);

		if (pnode->get_count() >= 1) {
			return;
		}

		if (pnode->get_nlink() == 0) {
			/* ��ǰ�ļ�û�й���·�������ͷ����DiskInode ���� �ͷ���ռ�õ������̿�*/
			filestoremanage.FileStoreReleaseDiskInodeArray(pnode->get_number());
			return;
		}
		if (pnode->get_update()) {
			/* ���и��±�ʶ ��Ҫд�����diskinode*/
			filestoremanage.FileStoreSetDiskInode(*pnode);
			/* �ӳ�д��ʶ�Ĵ��� */
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
	��������: �ļ��߼��̿鵽�����̿�ĵ�ַӳ�亯��
	�������ͣ�
		inode_index: �ڴ�Indoe�ڵ�����Ӧ�����
		lbn: ������ļ��߼��̿��
	����ֵ����: ������߼��̿������Ӧ�������̿��, �����-1, ���ʾ�ļ�����
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
