#include "FileSystem.h"
#include <stdio.h>

enum cmd_format {
	fformat=1,
	ls=1,
	mkdir,
	fcreate,
	fopen_ = 3,
	fclose_ = 2,
	fread_ = 3,
	fwrite_ = 4,
	fseek_ = 4,
	fdelete_ = 2,
	cd = 2
};

/* �ļ�������gbk���� ������ */
const char *help = 
"	fformat : ��ʽ�������ļ�������һ��Ŀ¼�ṹ\n\
	ls (-path) : �г���ǰĿ¼��·��Ŀ¼�µ��ļ��б�\n\
	mkdir [-dir] : ����һ��Ŀ¼\n\
	fcreate [-file] [-right] : ��������Ȩ�޴���һ���ļ� 0 - ֻ��  1 - ��д \n\
	fopen [-file] [-right] : ��������Ȩ�޴�һ���ļ� 0 - ֻ�� 1 - ��д \n\
	fclose [-fd] : �����ļ�����ر�һ���ļ�\n\
	fread [-fd] [-nbytes] : �����ļ��������nbytes�ֽڵ���Ϣ \n\
	fwrite [-fd] [-nbytes] [-string] : �����ļ����д��nbytes�ֽڵ���Ϣ \n\
	fseek [-fd] [-num] [-format] �����ļ�����������ļ�ָ��ƫ���� absolute - ������ʽ relative - �����ʽ \n\
	fdelete [-file] : ɾ��һ����ͨ�ļ� \n \
	cd [-path] : ����ĳ��Ŀ¼\n \
	exit : �˳�����\n";


int cmd_resolv(const string input, FileSys *filesys)
{
	vector<string> cmd;
	cmd.clear();
	int src_pos = 0;
	int space_pos = 0;
	const char * cmd_error = " ָ���ʽ����������help�鿴ָ���ʽ\n";
	int i = 0;
	/* ����ǰ���ո� */
	while (i < input.size() && input[i] == ' ') {
		src_pos = i + 1;
		i++;
	}
	for (; i < input.size(); i++) {

		if (input[i] == ' ') {
			space_pos = i;
			cmd.push_back(input.substr(src_pos, space_pos - src_pos));
			src_pos = space_pos + 1;
		}
		while (i < input.size() && input[i] == ' ') {
			src_pos = i + 1;
			i++;
		}
	}
	if(cmd.size() == 0){
		return 0;
	}
	else if (cmd[0] == "fformat") {
		if (cmd.size() == fformat) {
			int tmp = 0;
			fp.seekg(0, ios::beg);
			fp.write((char *)&tmp, sizeof(int));
			delete filesys;
			filesys = new FileSys;
			filesys->mkdir("bin");
			filesys->mkdir("etc");
			filesys->mkdir("home");
			filesys->mkdir("dev");
			filesys->cd("home");
			filesys->fcreate("texts", 1);
			filesys->fcreate("reports", 1);
			filesys->fcreate("photos", 1);
			FILE *fp = fopen("./readme", "rb");
			char buffer[100000];
			memset(buffer, 0, sizeof(buffer));
			int nbytes = fread(buffer, 1, sizeof(buffer) - 1, fp);
			fclose(fp);
			int pfile = filesys->fopen("texts", 1);
			filesys->fwrite_(pfile, nbytes, buffer);
			filesys->fclose(pfile);

			fp = fopen("./course_report", "rb");
			memset(buffer, 0, sizeof(buffer));
			nbytes = fread(buffer, 1, sizeof(buffer) - 1, fp);
			fclose(fp);
			pfile = filesys->fopen("reports", 1);
			filesys->fwrite_(pfile, nbytes, buffer);
			filesys->fclose(pfile);

			fp = fopen("./img.jpg", "rb");
			memset(buffer, 0, sizeof(buffer));
			nbytes = fread(buffer, 1, sizeof(buffer) - 1, fp);
			fclose(fp);
			pfile = filesys->fopen("photos", 1);
			filesys->fwrite_(pfile, nbytes, buffer);
			filesys->fclose(pfile);
			filesys->cd("/");
		}
		else
			cout << cmd[0] << cmd_error;
	}
	else if (cmd[0] == "ls") {
		if (cmd.size() == ls)
			filesys->ls();
		else if (cmd.size() == ls + 1)
			filesys->ls(cmd[1]);
		else
			cout << cmd[0] << cmd_error;
	}
	else if (cmd[0] == "mkdir") {
		if (cmd.size() != mkdir)
			cout << cmd[0] << cmd_error;
		else {
			bool succ = filesys->mkdir(cmd[1]);
			if (!succ)
				cout << "Ŀ¼����ʧ��\n";
		}
	}
	else if (cmd[0] == "fcreate") {
		if(cmd.size() != fcreate || atoi(cmd[2].c_str()) > 1)
			cout << cmd[0] << cmd_error;
		else {
			bool succ = filesys->fcreate(cmd[1], atoi(cmd[2].c_str()));
			if (!succ)
				cout << "�ļ�����ʧ��\n";
		}
	}
	else if (cmd[0] == "fopen") {
		if(cmd.size() != fopen_ || atoi(cmd[2].c_str()) > 1)
			cout << cmd[0] << cmd_error;
		else {
			int fd = filesys->fopen(cmd[1], atoi(cmd[2].c_str()));
			if (fd == -1)
				cout << "�ļ���ʧ��\n";
			else
				cout << "�ļ��򿪳ɹ�, ���Ϊ" << fd << endl;
		}
	}
	else if (cmd[0] == "fclose") {
		if (cmd.size() != fclose_)
			cout << cmd[0] << cmd_error;
		else
			filesys->fclose(atoi(cmd[1].c_str()));
	}
	else if (cmd[0] == "fread") {
		if (cmd.size() != fread_)
			cout << cmd[0] << cmd_error;
		else {
			char buffer[100000];
			memset(buffer, 0, sizeof(buffer));
			int bytes = filesys->fread_(atoi(cmd[1].c_str()), atoi(cmd[2].c_str()), buffer);
			cout << "�ļ�һ����ȡ" << bytes << "�ֽ�\n";
			cout << "����Ϊ: " ;
			for (int i = 0; i < bytes; i++) {
				if (buffer[i] == '\0')
					cout << " ";
				else if(buffer[i] != -1)
					cout << buffer[i];
				else {
					cout << "δ��ֹ������������ɲ���֪���󣬹��ж���ʾ";
					break;
				}
			}
			cout << endl;
		}
	}
	else if (cmd[0] == "fwrite") {
		if (cmd.size() != fwrite_)
			cout << cmd[0] << cmd_error;
		else {
			char buffer[100000];
			memset(buffer, 0, sizeof(buffer));
			strcpy(buffer, cmd[3].c_str());
			int bytes = filesys->fwrite_(atoi(cmd[1].c_str()), atoi(cmd[2].c_str()), buffer);
			cout << "�ļ�һ��д��" << bytes << "�ֽ�\n";
		}
	}
	else if (cmd[0] == "fseek") {
		if (cmd.size() != fseek_)
			cout << cmd[0] << cmd_error;
		else
			filesys->fseek_(atoi(cmd[1].c_str()), atoi(cmd[2].c_str()), cmd[3]);
	}
	else if (cmd[0] == "fdelete") {
		if (cmd.size() != fdelete_)
			cout << cmd[0] << cmd_error;
		else
			filesys->fdelete(cmd[1]);
	}
	else if (cmd[0] == "cd") {
		if (cmd.size() != cd)
			cout << cmd[0] << cmd_error;
		else
			filesys->cd(cmd[1]);
	}
	else if (cmd[0] == "exit") {
		fp.close();
		return 1;
	}
	else if (cmd[0] == "help") {
		cout << help;
	}
	else{
		cout << "������help�鿴ָ����Ϣ" << endl;
	}
	filesys->Getopenfiletable()->get_inode_table()->get_filestoremanage()->save();
	return 0;
}


int main()
{
	FileSys *filesys = new FileSys;
	while (1) {
		cout << "<" << filesys->GetCurDir() << "> : ";
		char buffer[1000];
		cin.getline(buffer, 100000, '\n');
		string info(buffer);
		/* ����������Ϣĩβ׷��һ���ո񣬱��ڴ���*/
		info += " ";
		int res = cmd_resolv(info, filesys);
		if (res == 1)
			break;
	}
}