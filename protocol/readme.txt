char cFileMd5Value[40] = {0};
CMD5::MD5((unsigned char *)m_pcRecvFileContent,m_iRecvFileLength,cFileMd5Value); //文件校验值计算