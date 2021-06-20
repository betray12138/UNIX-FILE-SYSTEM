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

/* 文件必须是gbk编码 ！！！ */
const char *help = 
"	fformat : 格式化整个文件卷，构造一般目录结构\n\
	ls (-path) : 列出当前目录或路径目录下的文件列表\n\
	mkdir [-dir] : 创建一个目录\n\
	fcreate [-file] [-right] : 根据所给权限创建一个文件 0 - 只读  1 - 读写 \n\
	fopen [-file] [-right] : 根据所给权限打开一个文件 0 - 只读 1 - 读写 \n\
	fclose [-fd] : 根据文件句柄关闭一个文件\n\
	fread [-fd] [-nbytes] : 根据文件句柄读出nbytes字节的信息 \n\
	fwrite [-fd] [-nbytes] [-string] : 根据文件句柄写入nbytes字节的信息 \n\
	fseek [-fd] [-num] [-format] 根据文件句柄，调整文件指针偏移量 absolute - 绝对形式 relative - 相对形式 \n\
	fdelete [-file] : 删除一个普通文件 \n \
	cd [-path] : 进入某个目录\n \
	exit : 退出程序\n";


int cmd_resolv(const string input, FileSys *filesys)
{
	vector<string> cmd;
	cmd.clear();
	int src_pos = 0;
	int space_pos = 0;
	const char * cmd_error = " 指令格式错误，请输入help查看指令格式\n";
	int i = 0;
	/* 消除前导空格 */
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
				cout << "目录创建失败\n";
		}
	}
	else if (cmd[0] == "fcreate") {
		if(cmd.size() != fcreate || atoi(cmd[2].c_str()) > 1)
			cout << cmd[0] << cmd_error;
		else {
			bool succ = filesys->fcreate(cmd[1], atoi(cmd[2].c_str()));
			if (!succ)
				cout << "文件创建失败\n";
		}
	}
	else if (cmd[0] == "fopen") {
		if(cmd.size() != fopen_ || atoi(cmd[2].c_str()) > 1)
			cout << cmd[0] << cmd_error;
		else {
			int fd = filesys->fopen(cmd[1], atoi(cmd[2].c_str()));
			if (fd == -1)
				cout << "文件打开失败\n";
			else
				cout << "文件打开成功, 句柄为" << fd << endl;
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
			cout << "文件一共读取" << bytes << "字节\n";
			cout << "内容为: " ;
			for (int i = 0; i < bytes; i++) {
				if (buffer[i] == '\0')
					cout << " ";
				else if(buffer[i] != -1)
					cout << buffer[i];
				else {
					cout << "未防止程序因乱码造成不可知错误，故中断显示";
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
			cout << "文件一共写入" << bytes << "字节\n";
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
		cout << "请输入help查看指令信息" << endl;
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
		/* 在命令行信息末尾追加一个空格，便于处理*/
		info += " ";
		int res = cmd_resolv(info, filesys);
		if (res == 1)
			break;
	}
}