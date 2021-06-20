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
	��������: ����memcpy�������� ��ɶ��ļ��Ķ�ȡ
	�������ͣ�
		dst_buf����������õ�λ��
		datadisk_index: �����̿����� Ҫ���0��ʼ
		bias: �����̿���ƫ����
		length����ȡ����
*/
void memcpy(void *dst_buf, int datadisk_index, int bias, unsigned int length)
{
	fp.seekg((datadisk_index + DATADISK_OFFSET) * BLOCK_SIZE + bias, ios::beg);
	fp.read((char *)dst_buf, length);
}

/*
	��������: ����memcpy�������� ��ɶ��ļ���д��
	�������ͣ�
		datadisk_index: �����̿����� Ҫ���0��ʼ
		dst_buf����������õ�λ��
		bias: �����̿���ƫ����
		length����ȡ����
*/
void memcpy(int datadisk_index, void *src_buf, int bias, unsigned int length)
{
	fp.seekg((datadisk_index + DATADISK_OFFSET) * BLOCK_SIZE + bias, ios::beg);
	fp.write((char *)src_buf, length);
}

/*
	��������Ŀ¼����Ŀ¼�ṹ ÿ��Ŀ¼�ṹ��Ӧ32�ֽڣ�4�ֽ�Ϊ��Ӧ��Inode���
	��Ա�������ͣ�
		m_inode: ÿ��Ŀ¼·����Ӧ��inode���
		m_name: ��ӦĿ¼�µ��ļ�����
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
	�������������̿�
	��Ա�������ͣ�
	���ܽ��ͣ�ÿ�������̿�512�ֽ�
*/
class DataDisk {
public:
	/*
		��������: ���ڵ�ǰ���ݿ��ж�Ӧ��Ŀ¼��ƥ��
		�������ͣ�
			dir_name: ��������ƥ���Ŀ¼��
			disk_index: �����̿����
		����ֵ���ͣ���ƥ�䲻���򷵻�-1. ���򷵻�ָ����һ��Ŀ¼DiskInode�����
	*/
	int dir_match(const string dir_name, const int disk_index)
	{
		/* ����Ŀ¼�ļ� �俪ʼ��4�ֽ�Ϊ��һ��Ŀ¼��ӦDiskInode����ţ����28�ֽ�Ϊ����Ŀ¼��*/
		for (int i = 0; i < BLOCK_SIZE / sizeof(DirectoryEntry); i++) {
			DirectoryEntry tmp;
			memcpy(&tmp, disk_index, i * sizeof(DirectoryEntry), sizeof(DirectoryEntry));
			if (tmp.get_name() == dir_name)
				return tmp.get_inode();
		}
		return -1;
	}

	/*
		��������: ��Ŀ¼�ļ������ݿ��е����ļ�����Ŀ¼��Ϣ����32�ֽ�
		�������ͣ�
			reg_name: ����Ǽǵ��ļ���
			diskinode_index: ������Ӧ��diskInode�����
			disk_index: �����̿����
		����ֵ���ͣ����Ǽǳɹ��򷵻�true ���Ǽ�ʧ���򷵻�false
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
		��������: �����ǰ���ݿ������ȫ��Ŀ¼��Ϣ
		�������ͣ�
			disk_index: �����̿����
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
		��������: �������ݿ������һ��Ŀ¼��¼��diskinode��ź�name ���ҽ������
		�������ͣ�
			inode: ʹ�����õ���ʽ���ص����һ���¼��diskinode���
			name: ʹ�����õ���ʽ�������һ���¼������
			disk_index: �����̿����
			clear: �����Ƿ����һ���¼���
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
				/* ������һ��Ŀ¼��¼ */
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
		��������: ��Ŀ¼�ļ������ݿ��и���Ŀ¼�ʹ��inode1,name1 �滻inode2
		�������ͣ�
			inode1: ������inode���
			name1: ������name
			inode2����������inode���
			disk_index: �����̿����
		����ֵ���ͣ��������ɹ��򷵻�true������ʧ���򷵻�false
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
	�����������Inode�ڵ�
	��Ա�������ͣ�
		d_mode: ״̬������:   7 д��Ȩ�� 8 ��ȡȨ��   12 �ļ���С��or���;���  13 �ļ�����ͨ�ļ�����Ŀ¼�ļ� 15 �ýڵ��Ƿ��ѷ���
		d_nlink: ���ļ���Ŀ¼���в�ͬ·����������
		d_size: �ļ���С�����ֽ�Ϊ��λ
		d_addr: �߼���ź������̿�ŵĶ�Ӧ��ϵ����   0-5С���ļ�  6-7�����ļ� 8-9�����ļ�
		padding: ����ռ��64�ֽڿռ�
	���ܽ��ͣ�ÿ��Inode�ڵ����һ���ļ�  ÿ��DiskInode��Ӧ64�ֽ�
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
		// index ��0��ʼ
		fp.seekg(Disk_OFFSET * BLOCK_SIZE + index * sizeof(DiskInode), ios::beg);
		// ��ȡd_mode
		fp.read((char *)&d_mode, sizeof(int));
		// ��ȡd_nlink
		fp.read((char *)&d_nlink, sizeof(int));
		// ��ȡd_size
		fp.read((char *)&d_size, sizeof(int));
		// ��ȡd_addr����
		for (int i = 0; i < sizeof(d_addr) / sizeof(int); i++) {
			fp.read((char *)&d_addr[i], sizeof(int));
		}
		// ��ȡpadding����
		for (int i = 0; i < sizeof(padding) / sizeof(int); i++) {
			fp.read((char *)&padding[i], sizeof(int));
		}
	}

	void Save(const int index)
	{
		// index ��0��ʼ
		fp.seekg(Disk_OFFSET * BLOCK_SIZE + index * sizeof(DiskInode), ios::beg);
		// д��d_mode
		fp.write((char *)&d_mode, sizeof(int));
		// д��d_nlink
		fp.write((char *)&d_nlink, sizeof(int));
		// д��d_size
		fp.write((char *)&d_size, sizeof(int));
		// д��d_addr����
		for (int i = 0; i < sizeof(d_addr) / sizeof(int); i++) {
			fp.write((char *)&d_addr[i], sizeof(int));
		}
		// д��padding����
		for (int i = 0; i < sizeof(padding) / sizeof(int); i++) {
			fp.write((char *)&padding[i], sizeof(int));
		}

	}

	/*
		��������: ��DiskInode��dmode�л�ȡ����Ȩ��
		�������ͣ�
			is_w: ͨ��ָ�뷽ʽ�����ļ����Ƿ���жԸ��ļ�д��Ȩ��
			is_r: ͨ��ָ�뷽ʽ�����ļ����Ƿ���жԸ��ļ�����Ȩ��
	*/
	void get_dmode_wr(bool *is_w, bool *is_r)
	{
		*is_w = d_mode & (1 << write);
		*is_r = d_mode & (1 << read);
	}

	/*
		��������: ΪDiskInode��dmode���ö�дȨ��
		�������ͣ�
			is_w: �����Ƿ���жԸ��ļ�д��Ȩ��
			is_r: �����Ƿ���жԸ��ļ�����Ȩ��
	*/
	void set_dmode_wr(const bool is_w, const bool is_r)
	{
		d_mode = d_mode | (int(is_w) << write) | (int(is_r) << read);
	}

	/*
		��������: ��DiskInode��dmode�л�ȡ�ļ���������
		�������ͣ�
			is_large: ͨ��ָ�뷽ʽ�����ļ��Ƿ�Ϊ���ͻ�����ļ�
	*/
	void get_dmode_file_size(bool *is_large)
	{
		*is_large = d_mode & (1 << large);
	}

	/*
		��������: ΪDiskInode��dmode�����ļ���������
		�������ͣ�
			is_large: �����ļ��Ƿ�Ϊ���ͻ�����ļ�
	*/
	void set_dmode_file_size(const bool is_large)
	{
		d_mode = d_mode | (int(is_large) << large);
	}

	/*
		��������: ��DiskInode��dmode�л�ȡ�ļ��Ƿ�ΪĿ¼�ļ�
		�������ͣ�
			is_dir: ͨ��ָ�뷽ʽ�����ļ��Ƿ�ΪĿ¼�ļ�
	*/
	void get_dmode_file_mode(bool *is_dir)
	{
		*is_dir = d_mode & (1 << dir);
	}

	/*
		��������: ΪDiskInode��dmode�����ļ��Ƿ�ΪĿ¼�ļ�
		�������ͣ�
			is_dir: �����ļ��Ƿ�ΪĿ¼�ļ�
	*/
	void set_dmode_file_mode(const bool is_dir)
	{
		d_mode = d_mode | (int(is_dir) << dir);
	}

	/*
		��������: ��DiskInode��dmode�л�ȡ�ýڵ��Ƿ��ѷ���
		�������ͣ�
			is_alloc: ͨ��ָ�뷽ʽ���ظýڵ��Ƿ��ѷ���
	*/
	void get_dmode_alloc(bool *is_alloc)
	{
		*is_alloc = d_mode & (1 << alloc);
	}

	/*
		��������: ΪDiskInode��dmode���ýڵ��Ƿ��ѷ���
		�������ͣ�
			is_alloc: �����ļ��Ƿ��ѷ���
	*/
	void set_dmode_alloc(const bool is_alloc)
	{
		d_mode = d_mode | (int(is_alloc) << alloc);
	}

	/*
		��������: ��DiskInode�л�ȡ�ļ���Ŀ¼���в�ͬ·����������
	*/
	int get_dnlink()
	{
		return d_nlink;
	}

	/*
		��������: ΪDiskInode���ò�ͬ·��������
		�������ͣ�
			nlink: �����õĲ�ͬ·��������
	*/
	void set_dnlink(const int nlink)
	{
		d_nlink = nlink;
	}

	/*
		��������: ��DiskInode�л�ȡ�ļ���С
	*/
	int get_dsize()
	{
		return d_size;
	}

	/*
		��������: ΪDiskInode�����ļ���С
		�������ͣ�
			size: �����õ��ļ���С
	*/
	void set_dsize(const int size)
	{
		d_size = size;
	}

	/*
		��������: ����[]������� ���ڻ�ȡ���޸�d_addr�е�ֵ
		�������ͣ�
			index: d_addr�е������±�
	*/
	int &operator [](const int index)
	{
		assert(index >= 0 && index < 10);
		return d_addr[index];
	}

	/*
		��������: ���ڵ�ǰDiskInode��d_addr�����Ӧ���ݿ��Ŀ¼��ƥ��
		�������ͣ�
			dir_name: ��������ƥ���Ŀ¼��
		����ֵ���ͣ���ƥ�䲻���򷵻�-1. ���򷵻�ָ����һ��Ŀ¼DiskInode�����
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
		��������: ��ǰDiskInodeΪĿ¼����Ŀ¼�������ȫ��Ŀ¼��Ϣ
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
		��������: ���ڻ�ȡ��ǰDiskInode�����һ��Ŀ¼��
		�������ͣ�
			last_inode: ʹ�����õ���ʽ���ص����һ���¼��diskinode���
			last_name: ʹ�����õ���ʽ�������һ���¼������
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
		��������: �ڵ�ǰdiskinode��Ӧ���������ݿ��и���Ŀ¼�ʹ��inode1,name1 �滻inode2
		�������ͣ�
			inode1: ������inode���
			name1: ������name
			inode2����������inode���
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
	���������ļ�ϵͳ������
	��Ա�������ͣ�
		s_nfree: ֱ�ӹ���Ŀ����̿�����
		s_free: ֱ�ӹ���Ŀ����̿�������
		s_ninode�� ֱ�ӹ���Ŀ������inode�ڵ�: ��Χ 1-100 ��ʼΪ0
		s_inode: ֱ�ӹ���Ŀ������inode������
		padding: ����ռ��1024�ֽڿռ�
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
		// ��ȡs_free����
		for (int i = 0; i < sizeof(s_free) / sizeof(int); i++) {
			fp.read((char *)&s_free[i], sizeof(int));
		}
		// ��ȡs_ninode
		fp.read((char *)&s_ninode, sizeof(int));
		// ��ȡs_inode����
		for (int i = 0; i < sizeof(s_inode) / sizeof(int); i++) {
			fp.read((char *)&s_inode[i], sizeof(int));
		}
		// ��ȡpadding����
		for (int i = 0; i < sizeof(padding) / sizeof(int); i++) {
			fp.read((char *)&padding[i], sizeof(int));
		}
	}

	void save()
	{
		fp.seekg(SP_OFFSET * BLOCK_SIZE, ios::beg);
		fp.write((char *)&s_nfree, sizeof(int));
		// д��s_free����
		for (int i = 0; i < sizeof(s_free) / sizeof(int); i++) {
			fp.write((char *)&s_free[i], sizeof(int));
		}
		// д��s_ninode
		fp.write((char *)&s_ninode, sizeof(int));
		// д��s_inode����
		for (int i = 0; i < sizeof(s_inode) / sizeof(int); i++) {
			fp.write((char *)&s_inode[i], sizeof(int));
		}
		// д��padding����
		for (int i = 0; i < sizeof(padding) / sizeof(int); i++) {
			fp.write((char *)&padding[i], sizeof(int));
		}


	}

	/*
		��������: ��s_inode�����з���һ�����е������ڵ㲢����
		�������ͣ�
			need_fill: ��s_ninode��Ϊ0ʱ����Ҫ����װ��s_inode����
		����ֵ���ͣ�����Ŀ�����������ڵ�
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
		��������: ����һ�������ڵ㲢�������s_inode������
		�������ͣ�
			index: ������յ���������ڵ��±�
	*/
	void inode_free(const int index)
	{
		if (s_ninode == max_node)
			return;
		s_inode[s_ninode++] = index;
	}

	/*
		��������: ��s_free�����з���һ�����е������̿鲢����
		�������ͣ�
			need_fill: ��s_nfree��Ϊ0ʱ����Ҫ����װ��s_free����
		����ֵ���ͣ�����Ŀ��������̿�
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
		��������: ����һ�������̿鲢����s_free������
		�������ͣ�
			index: ������յĿ����̿��±�
			need_fill: ��s_nfree��Ϊ100ʱ����Ҫ����װ��s_free����
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
	���������ڴ�Inode�ڵ� ÿ��Inode�ڵ�Ψһ��Ӧ���DiskInode�ڵ�
	��Ա�������ͣ�
		i_mode: ״̬������:   7 д��Ȩ�� 8 ��ȡȨ��
		i_update: �ڴ�inode���޸Ĺ� ��Ҫ������Ӧ���inode
		i_count: �������ڵ㵱ǰ��Ծ��ʵ����Ŀ����ֵΪ0����ýڵ���У��ͷŲ�д��
		i_nlink: ���ļ���Ŀ¼���в�ͬ·����������
		i_size: �ļ���С����λΪ�ֽ�
		i_addr: �ļ��߼���ź�������ת��������
		index: Inode�ڵ�����
	���ܽ��ͣ�ÿ��Inode�ڵ��Ӧһ�����inode�ڵ�
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
		��������: ��Inode��imode�л�ȡ����Ȩ��
		�������ͣ�
			is_w: ͨ��ָ�뷽ʽ�����ļ����Ƿ���жԸ��ļ�д��Ȩ��
			is_r: ͨ��ָ�뷽ʽ�����ļ����Ƿ���жԸ��ļ�����Ȩ��
	*/
	void get_imode_wr(bool *is_w, bool *is_r)
	{
		*is_w = i_mode & (1 << write);
		*is_r = i_mode & (1 << read);
	}

	/*
		��������: ΪInode��imode���ö�дȨ��
		�������ͣ�
			is_w: �����Ƿ���жԸ��ļ�д��Ȩ��
			is_r: �����Ƿ���жԸ��ļ�����Ȩ��
	*/
	void set_imode_wr(const bool is_w, const bool is_r)
	{
		i_mode = i_mode | (int(is_w) << write) | (int(is_r) << read);
	}

	/*
		��������: ��Inode��imode�л�ȡ��Ŀ¼�ļ�������ͨ�ļ�
		�������ͣ�
			is_dir: ͨ��ָ�뷽ʽ�����ļ��Ƿ���Ŀ¼�ļ�
	*/
	void get_imode_dir(bool *is_dir)
	{
		*is_dir = i_mode & (1 << dir);
	}

	/*
		��������: ΪInode��imode�����Ƿ���Ŀ¼�ļ�
		�������ͣ�
			is_dir: ������Ƿ���Ŀ¼�ļ�
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
		��������: ����[]������� ���ڻ�ȡ���޸�i_addr�е�ֵ
		�������ͣ�
			index: i_addr�е������±�
	*/
	int &operator [](const int index)
	{
		assert(index >= 0 && index < 10);
		return i_addr[index];
	}

};


/*
	�����������ļ��ṹ ���������û������Բ�ͬ��дȨ�� ��ͬһ�ļ� ָ���ڴ�Inode
	��Ա�������ͣ�
		f_flag: �ô��ļ��ṹ���ļ���д��Ҫ��   7 дȨ��  8 ��Ȩ��
		f_count: ����ָ��ýṹ������  �Ե�һ���̲����ڷ��ѽ��̵���� �����ֻ��Ϊ1
		f_inode: ָ���ڴ�Inode
		f_offset: �����ļ�ָ��
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
		��������: ���ļ��򿪽ṹ�л�ȡȨ������
		�������ͣ�
			is_w: ͨ��ָ�뷽ʽ�����ļ����Ƿ���жԸ��ļ�д��Ȩ��
			is_r: ͨ��ָ�뷽ʽ�����ļ����Ƿ���жԸ��ļ�����Ȩ��
	*/
	void get_dmode_wr(bool *is_w, bool *is_r)
	{
		*is_w = f_flag & (1 << write);
		*is_r = f_flag & (1 << read);
	}

	/*
		��������: ΪInode��imode���ö�дȨ��
		�������ͣ�
			is_w: �����Ƿ���жԸ��ļ�д��Ȩ��
			is_r: �����Ƿ���жԸ��ļ�����Ȩ��
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
	�����������Ƹ���һ�ζ�д��������
	��Ա�������ͣ�
		m_offset: ��ǰ��д�ļ����ֽ�ƫ����
		m_count: ��ǰ��ʣ��Ķ�д�ֽ�����
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