#include "lfs.h"
#include "lfs_util.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <dirent.h>
#include "cJSON.h"

#define file_num 100
lfs_t lfs;
lfs_file_t file;
FILE *pf1;

int user_provided_block_device_read(const struct lfs_config *c, lfs_block_t block, lfs_off_t off, void *buffer, lfs_size_t size)
{
    fseek(pf1, block * c->block_size + off, SEEK_SET); // 把光标指针偏移
    fread(buffer, sizeof(char), size, pf1);
    return 0;
}

int user_provided_block_device_prog(const struct lfs_config *c, lfs_block_t block, lfs_off_t off, const void *buffer, lfs_size_t size)
{
    fseek(pf1, block * c->block_size + off, SEEK_SET); // 把光标指针偏移
    fwrite(buffer, sizeof(char), size, pf1);
    return 0;
}

int user_provided_block_device_erase(const struct lfs_config *c, lfs_block_t block)
{
    unsigned char aaa[] = {0xff};
    fseek(pf1, block * c->block_size, SEEK_SET); // 把光标指针偏移
    for (int i = 0; i < 4096; i++)
    {
        fwrite(aaa, sizeof(char), 1, pf1);
    }
    return 0;
}

int user_provided_block_device_sync(const struct lfs_config *c)
{

    return 0;
}

// configuration of the filesystem is provided by this struct
struct lfs_config cfg = {
    // block device operations
    .read = user_provided_block_device_read,
    .prog = user_provided_block_device_prog,
    .erase = user_provided_block_device_erase,
    .sync = user_provided_block_device_sync,
    // block device configuration
    .read_size = 16,
    .prog_size = 16,
    .block_size = 4096,
    .block_count = 256,
    .cache_size = 16,
    .lookahead_size = 16,
    .block_cycles = 500,
};

int readJsonFile(const char *filename, char **jsonString)
{

    FILE *fp = fopen(filename, "r");
    if (fp == NULL)
    {
        printf("无法打开文件\n");
        return 1;
    }

    fseek(fp, 0, SEEK_END);
    long fileSize = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    *jsonString = (char *)malloc(fileSize + 1);
    if (*jsonString == NULL)
    {
        printf("内存分配失败\n");
        fclose(fp);
        return 0;
    }

    size_t bytesRead = fread(*jsonString, 1, fileSize, fp);
    (*jsonString)[bytesRead] = '\0';

    fclose(fp);
    return 1;
}

void logo_printf(void)
{
    printf(" __    __       _______..___  ___.      ___      .______     .___________.\n");
    printf("|  |  |  |     /       ||   \\/   |     /   \\     |   _  \\    |           |\n");
    printf("|  |  |  |    |   (----`|  \\  /  |    /  ^  \\    |  |_)  |   `---|  |----`\n");
    printf("|  |  |  |     \\   \\    |  |\\/|  |   /  /_\\  \\   |      /        |  |     \n");
    printf("|  `--'  | .----)   |   |  |  |  |  /  _____  \\  |  |\\  \\----.   |  |     \n");
    printf(" \\______/  |_______/    |__|  |__| /__/     \\__\\ | _| `._____|   |__|     \n");
    printf("----------------------------优智云家LFS镜像工具----------------------------\n");
    printf("输出文件的路径:./out\n");
}

int main(void)
{
    logo_printf();
    char *jsonString = NULL;
    if (readJsonFile("./files/usmartgo_lfs_config.json", &jsonString))
    {
        // 打印json数据
        // printf("%s\n", jsonString);
        // 1.创建cjson指针
        cJSON *json = NULL;
        // 2.解析整段JSON数据，返回cJSON指针
        json = cJSON_Parse(jsonString);
        if (json == 0)
        {
            printf("error:%s\n", cJSON_GetErrorPtr());
        }
        else
        {
            printf("JSON fiel parsing successful\n");
            cJSON *sector_size = cJSON_GetObjectItem(json, "sector_size");
            if (cJSON_IsNumber(sector_size))
            {
                // printf("sector_size = %d\n", sector_size->valueint);
                cfg.block_size = sector_size->valueint;
            }
            cJSON *sector_num = cJSON_GetObjectItem(json, "sector_num");
            if (cJSON_IsNumber(sector_num))
            {
                // printf("sector_num = %d\n", sector_num->valueint);
                cfg.block_count = sector_num->valueint;
            }
            cJSON_Delete(json);
        }

        free(jsonString);
    }

    DIR *dir;
    struct dirent *ent;
    // 将所需要查询的文件夹路径放在下方即可
    dir = opendir("./files");
    // 注意Windows下路径用'/'隔开
    if (dir == NULL)
    {
        printf("无法打开文件夹\n");
        return 1;
    }

    // 删除并创建配置文件
    remove("./out/usmartgo_lfs_mirror.bin");
    // printf("remove\n");
    pf1 = fopen("./out/usmartgo_lfs_mirror.bin", "wb+");
    unsigned char temp1[] = {0xff};
    unsigned int p = cfg.block_size * cfg.block_count;
    for (int i = 0; i < p; i++)
    {
        fwrite(temp1, sizeof(char), 1, pf1);
    }

    printf("单个扇区的大小为%d字节,有%d块扇区,总大小为%dkb:\n", cfg.block_size, cfg.block_count, cfg.block_size * cfg.block_count / 1024);
    int err = lfs_mount(&lfs, &cfg); // 挂载文件系统
    // 挂载文件系统失败再次挂载
    if (err)
    {
        lfs_format(&lfs, &cfg);
        lfs_mount(&lfs, &cfg);
    }

    while ((ent = readdir(dir)) != NULL)
    {
        if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0)
        {
            continue;
        }
        // printf("\n");
        // printf("file name :%s,file length :%d\n", ent->d_name, ent->d_namlen);

        // 拼接文件路径
        char my_str[] = "./files/";
        strcat(my_str, ent->d_name);
        // printf("my_str:%s\n", my_str);
        FILE *pf = fopen(my_str, "rb+");
        // 判断是否打开成功
        if (pf == NULL)
        {
            perror("fopen");
            printf("file open error\n");
            return 1;
        }

        fseek(pf, 0, SEEK_END);    // 将文件指针指向该文件的最后
        int file_size = ftell(pf); // 根据指针位置，此时可以算出文件的字符数
        // printf("The number of characters in the file is ----------%d\n", file_size);

        unsigned char *num2 = (unsigned char *)malloc(sizeof(unsigned char) * file_size);
        fseek(pf, 0, SEEK_SET);                   // 重新将指针指向文件首部
        fread(num2, sizeof(char), file_size, pf); // 读取数据到数组里面
        // for (int i = 0; i < file_size; i++)
        // {
        //     printf("0x%x\n", num2[i]);
        // }
        fclose(pf);
        pf = NULL;

        lfs_file_open(&lfs, &file, ent->d_name, LFS_O_RDWR | LFS_O_CREAT);
        lfs_file_write(&lfs, &file, num2, file_size); // 把值写入文件系统
        printf("Writing %s file,The file size is: %d\n", ent->d_name, file_size);
        printf("%s write completed..........\n\n", ent->d_name);

        // 读取当前的值
        unsigned char *temp1 = (unsigned char *)malloc(sizeof(unsigned char) * file_size);
        lfs_file_rewind(&lfs, &file); // 把文件指针的位置回到文件的起始位置。
        lfs_file_read(&lfs, &file, temp1, file_size);
        // for (int i = 0; i < file_size; i++)
        // {
        //     printf("lfs_file_read----%d\t0x%x\t%c\n", temp1[i], temp1[i], &temp1[i]);
        // }
        lfs_file_close(&lfs, &file); // 需要关闭文件，不然不会更新存储
        free(num2);
        free(temp1);
        num2 = NULL;
        temp1 = NULL;
    }
    lfs_unmount(&lfs); // 卸载文件系统，释放资源
    closedir(dir);
    fclose(pf1); // 关闭文件流
    system("pause");
}
