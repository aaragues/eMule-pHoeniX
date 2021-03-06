/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 2; tab-width: 2 -*- */
/* GeoIPUpdate.c
 *
 * Copyright (C) 2004 MaxMind LLC
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "GeoIPCity.h"
#include "GeoIP.h"
#include "GeoIPUpdate.h"

#include "global.h"
#include "md5.h"
#include <sys/types.h>
#ifndef _WIN32
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>
#else
#include <windows.h>
#include <winsock.h>
#endif
#include <zlib.h>
#include <time.h>
#include <stdio.h>
//#include <unistd.h> // [TPT] - IP Country

extern void _setup_dbfilename();
extern char *_full_path_to(const char *file_name);

const char *GeoIPUpdateHost = "updates.maxmind.com";
const char *GeoIPHTTPRequest = "GET /app/update?license_key=%s&md5=%s HTTP/1.0\nHost: updates.maxmind.com\n\n";
const char *GeoIPHTTPRequestMD5 = "GET /app/update_secure?db_md5=%s&challenge_md5=%s&user_id=%s&edition_id=%s HTTP/1.0\nHost: updates.maxmind.com\n\n";

/* messages */
const char *NoCurrentDB = "%s can't be opened, proceeding to download database\n";
const char *MD5Info = "MD5 Digest of installed database is %s\n";
const char *SavingGzip = "Saving gzip file to %s ... ";
const char *WritingFile = "Writing uncompressed data to %s ...";

/* TODO replace printf with GeoIP_printf - we need someway of having vargs with GeoIP_printf */

void GeoIP_printf(void (*f)(char *), const char *str) {
	char * f_str;
	f_str = malloc(strlen(str)+1);
	strcpy(f_str,str);
	if (f != NULL)
		(*f)(f_str);
}

short int GeoIP_update_database (char * license_key, int verbose, void (*f)( char *)) {
	struct hostent *hostlist;
	size_t sock, len; // [TPT] - IP Country
	char * buf;
	struct sockaddr_in sa;
	int offset = 0, err;
	int block_size = 1024;
	char * request_uri;
	char *uncompr = NULL, *compr;
	unsigned long comprLen;
	FILE *comp_fh, *cur_db_fh, *gi_fh;
	gzFile gz_fh;
	char * file_path_gz, * file_path_test;
	MD5_CTX context;
	unsigned char buffer[1024], digest[16];
	char hex_digest[32] = "00000000000000000000000000000000";
	unsigned int i;
	char *f_str;
	GeoIP * gi;
	char * db_info;

	_setup_dbfilename();

	/* get MD5 of current GeoIP database file */
	if ((cur_db_fh = fopen (GeoIPDBFileName[GEOIP_COUNTRY_EDITION], "rb")) == NULL) {
		f_str = malloc(strlen(NoCurrentDB) + strlen(GeoIPDBFileName[GEOIP_COUNTRY_EDITION]) - 1);
		sprintf(f_str,NoCurrentDB, GeoIPDBFileName[GEOIP_COUNTRY_EDITION]);
		if (f != NULL)
			(*f)(f_str);
	} else {
		MD5Init(&context);
		while ((len = fread (buffer, 1, 1024, cur_db_fh)) > 0)
			MD5Update (&context, buffer, len);
		MD5Final (digest, &context);
		fclose (cur_db_fh);
		for (i = 0; i < 16; i++)
			sprintf (&hex_digest[2*i], "%02x", digest[i]);
		f_str = malloc(strlen(MD5Info) + strlen(hex_digest) - 1);
		sprintf(f_str, MD5Info, hex_digest);
		if (f != NULL)
			(*f)(f_str);
	}

	hostlist = gethostbyname(GeoIPUpdateHost);

	if (hostlist == NULL)
		return GEOIP_DNS_ERR;

	if (hostlist->h_addrtype != AF_INET)
		return GEOIP_NON_IPV4_ERR;

	if((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		return GEOIP_SOCKET_OPEN_ERR;
	}

	memset(&sa, 0, sizeof(struct sockaddr_in));
	sa.sin_port = htons(80);
	memcpy(&sa.sin_addr, hostlist->h_addr_list[0], hostlist->h_length);
	sa.sin_family = AF_INET;

	if (verbose == 1)
		GeoIP_printf(f,"Connecting to MaxMind GeoIP Update server\n");

	/* Download gzip file */
	if (connect(sock, (struct sockaddr *)&sa, sizeof(struct sockaddr))< 0)
		return GEOIP_CONNECTION_ERR;

	request_uri = malloc(sizeof(char) * (strlen(license_key) + strlen(GeoIPHTTPRequest)+36));
	if (request_uri == NULL)
		return GEOIP_OUT_OF_MEMORY_ERR;
	sprintf(request_uri,GeoIPHTTPRequest,license_key, hex_digest);
	send(sock, request_uri, strlen(request_uri),0);
	free(request_uri);

	buf = malloc(sizeof(char) * block_size);
	if (buf == NULL)
		return GEOIP_OUT_OF_MEMORY_ERR;

	if (verbose == 1)
		GeoIP_printf(f,"Downloading gzipped GeoIP Database...\n");

	for (;;) {
		int amt;
		amt = recv(sock, &buf[offset], block_size,0);
		if (amt == 0) {
			break;
		} else if (amt == -1) {
			free(buf);
			return GEOIP_SOCKET_READ_ERR;
		}
		offset += amt;
		buf = realloc(buf, offset+block_size);
		if (buf == NULL)
			return GEOIP_OUT_OF_MEMORY_ERR;
	}

	compr = strstr(buf, "\r\n\r\n") + 4;
	comprLen = offset + buf - compr;

	if (strstr(compr, "License Key Invalid") != NULL) {
		if (verbose == 1)
			GeoIP_printf(f,"Failed\n");
		free(buf);
		return GEOIP_LICENSE_KEY_INVALID_ERR;
	} else if (strstr(compr, "No new updates available") != NULL) {
		free(buf);
		return GEOIP_NO_NEW_UPDATES;
	}

	if (verbose == 1)
		GeoIP_printf(f,"Done\n");

	/* save gzip file */
	file_path_gz = malloc(sizeof(char) * (strlen(GeoIPDBFileName[GEOIP_COUNTRY_EDITION]) + 4));
	if (file_path_gz == NULL)
		return GEOIP_OUT_OF_MEMORY_ERR;
	strcpy(file_path_gz,GeoIPDBFileName[GEOIP_COUNTRY_EDITION]);
	strcat(file_path_gz,".gz");
	if (verbose == 1) {
		f_str = malloc(strlen(SavingGzip) + strlen(file_path_gz) - 1);
		sprintf(f_str,SavingGzip,file_path_gz);
		if (f != NULL)
			(*f)(f_str);
	}
	comp_fh = fopen(file_path_gz, "wb");

	if(comp_fh == NULL) {
		free(buf);
		return GEOIP_GZIP_IO_ERR;
	}

	fwrite(compr, 1, comprLen, comp_fh);
	fclose(comp_fh);
	free(buf);

	if (verbose == 1)
		GeoIP_printf(f,"Done\n");

	if (verbose == 1)
		GeoIP_printf(f,"Uncompressing gzip file ... ");

	/* uncompress gzip file */
	offset = 0;
	gz_fh = gzopen(file_path_gz, "rb");
	free(file_path_gz);
	for (;;) {
		int amt;
		uncompr = realloc(uncompr, offset+block_size);
		if (uncompr == NULL)
			return GEOIP_OUT_OF_MEMORY_ERR;
		amt = gzread(gz_fh, &uncompr[offset], block_size);
		if (amt == -1) {
			gzclose(gz_fh);
			return GEOIP_GZIP_READ_ERR;
		}
		if (amt == 0) {
			break;
		}
		offset += amt;
	}
	gzclose(gz_fh);
	unlink(file_path_gz);

	if (verbose == 1)
		GeoIP_printf(f,"Done\n");

	if (verbose == 1) {
		f_str = malloc(strlen(WritingFile) + strlen(GeoIPDBFileName[GEOIP_COUNTRY_EDITION]) - 1);
		sprintf(f_str,WritingFile,GeoIPDBFileName[GEOIP_COUNTRY_EDITION]);
	}

	/* write uncompressed GeoIP.dat.test file */
	file_path_test = malloc(sizeof(char) * (strlen(GeoIPDBFileName[GEOIP_COUNTRY_EDITION]) + 6));
	if (file_path_test == NULL)
		return GEOIP_OUT_OF_MEMORY_ERR;
	strcpy(file_path_test,GeoIPDBFileName[GEOIP_COUNTRY_EDITION]);
	strcat(file_path_test,".test");
	gi_fh = fopen(file_path_test, "wb");

	if(gi_fh == NULL) {
		free(uncompr);
		return GEOIP_TEST_IO_ERR;
	}

	fwrite(uncompr, 1, offset, gi_fh);
	fclose(gi_fh);
	free(uncompr);

	/* sanity check */
	gi = GeoIP_open(file_path_test, GEOIP_STANDARD);

	if (verbose == 1)
		GeoIP_printf(f,"Performing santity checks ... ");

	if (gi == NULL) {
		GeoIP_printf(f,"Error opening sanity check database\n");
		return GEOIP_SANITY_OPEN_ERR;
	}

	/* this checks to make sure the files is complete, since info is at the end */
	/* dependent on future databases having MaxMind in info */
	if (verbose == 1)
		GeoIP_printf(f,"database_info  ");
	db_info = GeoIP_database_info(gi);
	if (db_info == NULL) {
		GeoIP_delete(gi);
		if (verbose == 1)
			GeoIP_printf(f,"FAIL\n");
		return GEOIP_SANITY_INFO_FAIL;
	}
	if (strstr(db_info, "MaxMind") == NULL) {
		free(db_info);
		GeoIP_delete(gi);
		if (verbose == 1)
			GeoIP_printf(f,"FAIL\n");
		return GEOIP_SANITY_INFO_FAIL;
	}
	free(db_info);
	if (verbose == 1)
		GeoIP_printf(f,"PASS  ");

	/* this performs an IP lookup test of a US IP address */
	if (verbose == 1)
		GeoIP_printf(f,"lookup  ");
	if (strcmp(GeoIP_country_code_by_addr(gi,"24.24.24.24"), "US") != 0) {
		GeoIP_delete(gi);
		if (verbose == 1)
			GeoIP_printf(f,"FAIL\n");
		return GEOIP_SANITY_LOOKUP_FAIL;
	}
	GeoIP_delete(gi);
	if (verbose == 1)
		GeoIP_printf(f,"PASS\n");

	/* install GeoIP.dat.test -> GeoIP.dat */
	err = rename(file_path_test, GeoIPDBFileName[GEOIP_COUNTRY_EDITION]);
	if (err != 0) {
		GeoIP_printf(f,"GeoIP Install error while renaming file\n");
		return GEOIP_RENAME_ERR;
	}

	if (verbose == 1)
		GeoIP_printf(f,"Done\n");

	return 0;
}

short int GeoIP_update_database_general (char * user_id,char * license_key,char *data_base_type, int verbose,char ** client_ipaddr, void (*f)( char *)) {
	struct hostent *hostlist;
	int sock, len;
	char * buf;
	struct sockaddr_in sa;
	int offset = 0, err;
	int block_size = 1024;
	char * request_uri;
	char *uncompr = NULL, *compr;
	unsigned long comprLen;
	FILE *comp_fh, *cur_db_fh, *gi_fh;
	gzFile gz_fh;
	char * file_path_gz, * file_path_test;
	MD5_CTX context;
	MD5_CTX context2;
	unsigned char buffer[1024], digest[16] ,digest2[16];
	char hex_digest[33] = "00000000000000000000000000000000";
	char hex_digest2[33] = "00000000000000000000000000000000";
	unsigned int i;
	char *f_str;
	GeoIP * gi;
	char * db_info;
	char *ipaddress;
	char *geoipfilename;
	char *tmpstr;
	int dbtype;
	int lookupresult = 1;

	hostlist = gethostbyname(GeoIPUpdateHost);

	if (hostlist == NULL)
		return GEOIP_DNS_ERR;

	if (hostlist->h_addrtype != AF_INET)
		return GEOIP_NON_IPV4_ERR;
	if((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		return GEOIP_SOCKET_OPEN_ERR;
	}

	memset(&sa, 0, sizeof(struct sockaddr_in));
	sa.sin_port = htons(80);
	memcpy(&sa.sin_addr, hostlist->h_addr_list[0], hostlist->h_length);
	sa.sin_family = AF_INET;
	if (connect(sock, (struct sockaddr *)&sa, sizeof(struct sockaddr))< 0)
		return GEOIP_CONNECTION_ERR;
	request_uri = malloc(sizeof(char) * (strlen(license_key) + strlen(GeoIPHTTPRequestMD5)+1036));
	if (request_uri == NULL)
		return GEOIP_OUT_OF_MEMORY_ERR;

	/* get the file name from a web page using the product id */
	sprintf(request_uri,"GET /app/update_getfilename?product_id=%s HTTP/1.0\nHost: %s\n\n",data_base_type,GeoIPUpdateHost);
	if (verbose == 1) {
		printf("sending request %s \n",request_uri);
	}
	send(sock, request_uri, strlen(request_uri),0); /* send the request */
	free(request_uri);
	buf = malloc(sizeof(char) * (block_size+4));
	if (buf == NULL)
		return GEOIP_OUT_OF_MEMORY_ERR;
	offset = 0;
	for (;;){
		int amt;
		amt = recv(sock, &buf[offset], block_size,0); 
		if (amt == 0){
			break;
		} else if (amt == -1) {
			free(buf);
			return GEOIP_SOCKET_READ_ERR;
		}
		offset += amt;
		buf = realloc(buf, offset + block_size + 4);
	}
	buf[offset] = 0;
	offset = 0;
	tmpstr = strstr(buf, "\r\n\r\n") + 4;
	geoipfilename = _full_path_to(tmpstr);
	free(buf);

	/* print the database product id and the database filename */
	if (verbose == 1){
		printf("database product id %s database file name %s \n",data_base_type,geoipfilename);
	}
	_setup_dbfilename();

	/* get MD5 of current GeoIP database file */
	if ((cur_db_fh = fopen (geoipfilename, "rb")) == NULL) {
		f_str = malloc(strlen(NoCurrentDB) + strlen(geoipfilename) - 1);
		sprintf(f_str,NoCurrentDB, geoipfilename);

		if (f != NULL)
			(*f)(f_str);
	} else {
		MD5Init(&context);
		while ((len = fread (buffer, 1, 1024, cur_db_fh)) > 0)
			MD5Update (&context, buffer, len);
		MD5Final (digest, &context);
		fclose (cur_db_fh);
		for (i = 0; i < 16; i++)
			sprintf (&hex_digest[2*i], "%02x", digest[i]);
		f_str = malloc(strlen(MD5Info) + strlen(hex_digest) - 1);
		sprintf(f_str, MD5Info, hex_digest);
		if (f != NULL)
			(*f)(f_str);
	}
	if (verbose == 1) {
		printf("MD5 sum of database %s is %s \n",geoipfilename,hex_digest);
	}
	if (client_ipaddr[0] == NULL) {
		/* We haven't gotten our IP address yet, so let's request it */
		if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
			return GEOIP_SOCKET_OPEN_ERR;
		}

		memset(&sa, 0, sizeof(struct sockaddr_in));
		sa.sin_port = htons(80);
		memcpy(&sa.sin_addr, hostlist->h_addr_list[0], hostlist->h_length);
		sa.sin_family = AF_INET;

		if (verbose == 1)
			GeoIP_printf(f,"Connecting to MaxMind GeoIP Update server\n");

		/* Download gzip file */
		if (connect(sock, (struct sockaddr *)&sa, sizeof(struct sockaddr))< 0)
			return GEOIP_CONNECTION_ERR;
		request_uri = malloc(sizeof(char) * (strlen(license_key) + strlen(GeoIPHTTPRequestMD5)+1036));
		if (request_uri == NULL)
			return GEOIP_OUT_OF_MEMORY_ERR;

		/* get client ip address from MaxMind web page */
		sprintf(request_uri,"GET /app/update_getipaddr HTTP/1.0\nHost: %s\n\n",GeoIPUpdateHost);
		send(sock, request_uri, strlen(request_uri),0); /* send the request */
		if (verbose == 1) {
			printf("sending request %s", request_uri);
		}
		free(request_uri);
		buf = malloc(sizeof(char) * (block_size+1));
		if (buf == NULL)
			return GEOIP_OUT_OF_MEMORY_ERR;
		offset = 0;

		for (;;){
			int amt;
			amt = recv(sock, &buf[offset], block_size,0); 
			if (amt == 0) {
				break;
			} else if (amt == -1) {
				free(buf);
				return GEOIP_SOCKET_READ_ERR;
			}
			offset += amt;
			buf = realloc(buf, offset+block_size+1);
		}

		buf[offset] = 0;
		offset = 0;
		ipaddress = strstr(buf, "\r\n\r\n") + 4; /* get the ip address */
		ipaddress = malloc(strlen(strstr(buf, "\r\n\r\n") + 4)+5);
		strcpy(ipaddress,strstr(buf, "\r\n\r\n") + 4);
		client_ipaddr[0] = ipaddress;
		if (verbose == 1) {
			printf("client ip address: %s\n",ipaddress);
		}
		free(buf);
		close(sock);
	}

	ipaddress = client_ipaddr[0];

	/* make a md5 sum of ip address and license_key and store it in hex_digest2 */
	request_uri = malloc(sizeof(char) * 2036);
	MD5Init(&context2);
	MD5Update (&context2, license_key, 12);//add license key to the md5 sum
	MD5Update (&context2, ipaddress, strlen(ipaddress));//add ip address to the md5 sum
	MD5Final (digest2, &context2);
	for (i = 0; i < 16; i++)
		sprintf (&hex_digest2[2*i], "%02x", digest2[i]);// change the digest to a hex digest
	if (verbose == 1) {
		printf("md5sum of ip address and license key is %s \n",hex_digest2);
	}

	/* send the request using the user id,product id, 
	 * md5 sum of the prev database and 
	 * the md5 sum of the license_key and ip address */
	if((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		return GEOIP_SOCKET_OPEN_ERR;
	}
	memset(&sa, 0, sizeof(struct sockaddr_in));
	sa.sin_port = htons(80);
	memcpy(&sa.sin_addr, hostlist->h_addr_list[0], hostlist->h_length);
	sa.sin_family = AF_INET;
	if (connect(sock, (struct sockaddr *)&sa, sizeof(struct sockaddr))< 0)
		return GEOIP_CONNECTION_ERR;
	sprintf(request_uri,GeoIPHTTPRequestMD5,hex_digest,hex_digest2,user_id,data_base_type);
	send(sock, request_uri, strlen(request_uri),0);
	if (verbose == 1) {
		printf("sending request %s\n",request_uri);
	}

	free(request_uri);

	offset = 0;
	buf = malloc(sizeof(char) * block_size);
	if (buf == NULL)
		return GEOIP_OUT_OF_MEMORY_ERR;

	if (verbose == 1)
		GeoIP_printf(f,"Downloading gzipped GeoIP Database...\n");

	for (;;) {
		int amt;
		amt = recv(sock, &buf[offset], block_size,0);

		if (amt == 0) {
			break;
		} else if (amt == -1) {
			free(buf);
			return GEOIP_SOCKET_READ_ERR;
		}
		offset += amt;
		buf = realloc(buf, offset+block_size);
		if (buf == NULL)
			return GEOIP_OUT_OF_MEMORY_ERR;
	}

	compr = strstr(buf, "\r\n\r\n") + 4;
	comprLen = offset + buf - compr;

	if (strstr(compr, "License Key Invalid") != NULL) {
		if (verbose == 1)
			GeoIP_printf(f,"Failed\n");
		free(buf);
		return GEOIP_LICENSE_KEY_INVALID_ERR;
	} else if (strstr(compr, "No new updates available") != NULL) {
		free(buf);
		if (verbose == 1) {
			printf("no new updates\n");
		}
		return GEOIP_NO_NEW_UPDATES;
	}

	if (verbose == 1)
		GeoIP_printf(f,"Done\n");

	/* save gzip file */
	file_path_gz = malloc(sizeof(char) * (strlen(geoipfilename) + 4));

	if (file_path_gz == NULL)
		return GEOIP_OUT_OF_MEMORY_ERR;
	strcpy(file_path_gz,geoipfilename);
	strcat(file_path_gz,".gz");
	if (verbose == 1) {
		f_str = malloc(strlen(SavingGzip) + strlen(file_path_gz) - 1);
		sprintf(f_str,SavingGzip,file_path_gz);
		if (f != NULL)
			(*f)(f_str);
	}
	comp_fh = fopen(file_path_gz, "wb");

	if(comp_fh == NULL) {
		free(buf);
		return GEOIP_GZIP_IO_ERR;
	}

	fwrite(compr, 1, comprLen, comp_fh);
	fclose(comp_fh);
	free(buf);

	if (verbose == 1) {
		printf("download data to a gz file named %s \n",file_path_gz);
	}
	if (verbose == 1)
		GeoIP_printf(f,"Done\n");

	if (verbose == 1)
		GeoIP_printf(f,"Uncompressing gzip file ... ");

	/* uncompress gzip file */
	offset = 0;
	gz_fh = gzopen(file_path_gz, "rb");
	for (;;) {
		int amt;
		uncompr = realloc(uncompr, offset+block_size);
		if (uncompr == NULL)
			return GEOIP_OUT_OF_MEMORY_ERR;
		amt = gzread(gz_fh, &uncompr[offset], block_size);
		if (amt == -1) {
			gzclose(gz_fh);
			return GEOIP_GZIP_READ_ERR;
		}
		if (amt == 0) {
			break;
		}
		offset += amt;
	}
	gzclose(gz_fh);
	unlink(file_path_gz);
	free(file_path_gz);

	if (verbose == 1)
		GeoIP_printf(f,"Done\n");

	if (verbose == 1) {
		f_str = malloc(strlen(WritingFile) + strlen(geoipfilename) - 1);
		sprintf(f_str,WritingFile,geoipfilename);
	}

	/* write uncompressed GeoIP.dat.test file */
	file_path_test = malloc(sizeof(char) * (strlen(geoipfilename) + 6));
	if (file_path_test == NULL)
		return GEOIP_OUT_OF_MEMORY_ERR;
	strcpy(file_path_test,geoipfilename);
	strcat(file_path_test,".test");
	gi_fh = fopen(file_path_test, "wb");

	if(gi_fh == NULL) {
		free(uncompr);
		return GEOIP_TEST_IO_ERR;
	}

	fwrite(uncompr, 1, offset, gi_fh);
	fclose(gi_fh);
	free(uncompr);
	if (verbose == 1) {
		printf("file path test is %s \n",file_path_test);
	}

	/* sanity check */
	gi = GeoIP_open(file_path_test, GEOIP_STANDARD);

	if (verbose == 1)
		GeoIP_printf(f,"Performing santity checks ... ");

	if (gi == NULL) {
		GeoIP_printf(f,"Error opening sanity check database\n");
		return GEOIP_SANITY_OPEN_ERR;
	}


	/* get the database type */
	dbtype = GeoIP_database_edition(gi);
	if (verbose == 1) {
		printf("data base type is %d \n",dbtype);
	}

	/* this checks to make sure the files is complete, since info is at the end
		 dependent on future databases having MaxMind in info (ISP and Organization databases currently don't have info string */

	if ((dbtype != GEOIP_ISP_EDITION)&&
			(dbtype != GEOIP_ORG_EDITION)) {
		if (verbose == 1)
			GeoIP_printf(f,"database_info  ");
		db_info = GeoIP_database_info(gi);
		if (db_info == NULL) {
			GeoIP_delete(gi);
			if (verbose == 1)
				GeoIP_printf(f,"FAIL null\n");
			return GEOIP_SANITY_INFO_FAIL;
		}
		if (strstr(db_info, "MaxMind") == NULL) {
			free(db_info);
			GeoIP_delete(gi);
			if (verbose == 1)
				GeoIP_printf(f,"FAIL maxmind\n");
			return GEOIP_SANITY_INFO_FAIL;
		}
		free(db_info);
		if (verbose == 1)
			GeoIP_printf(f,"PASS  ");
	}

	/* this performs an IP lookup test of a US IP address */
	if (verbose == 1)
		GeoIP_printf(f,"lookup  ");
	if (dbtype == GEOIP_NETSPEED_EDITION) {
		int netspeed = GeoIP_id_by_name(gi,"24.24.24.24");
		lookupresult = 0;
		if (netspeed == GEOIP_CABLEDSL_SPEED){
			lookupresult = 1;
		}
	}
	if (dbtype == GEOIP_COUNTRY_EDITION) {
		/* if data base type is country then call the function
		 * named GeoIP_country_code_by_addr */
		lookupresult = 1;
		if (strcmp(GeoIP_country_code_by_addr(gi,"24.24.24.24"), "US") != 0) {
			lookupresult = 0;
		}
		if (verbose == 1) {
			GeoIP_printf(f,"testing GEOIP_COUNTRY_EDITION\n");
		}
	}
	if (dbtype == GEOIP_REGION_EDITION_REV1) {
		/* if data base type is region then call the function
		 * named GeoIP_region_by_addr */
		GeoIPRegion *r = GeoIP_region_by_addr(gi,"24.24.24.24");
		lookupresult = 0;
		if (r != NULL) {
			lookupresult = 1;
			free(r);
		}
		if (verbose == 1) {
			GeoIP_printf(f,"testing GEOIP_REGION_EDITION\n");
		}
	}
	if (dbtype == GEOIP_CITY_EDITION_REV1) {
		/* if data base type is city then call the function
		 * named GeoIP_record_by_addr */
		GeoIPRecord *r = GeoIP_record_by_addr(gi,"24.24.24.24");
		lookupresult = 0;
		if (r != NULL) {
			lookupresult = 1;
			free(r);
		}
		if (verbose == 1) {
			GeoIP_printf(f,"testing GEOIP_CITY_EDITION\n");
		}
	}
	if ((dbtype == GEOIP_ISP_EDITION)||
			(dbtype == GEOIP_ORG_EDITION)) {
		/* if data base type is isp or org then call the function
		 * named GeoIP_org_by_addr */
		GeoIPRecord *r = GeoIP_org_by_addr(gi,"24.24.24.24");
		lookupresult = 0;
		if (r != NULL) {
			lookupresult = 1;
			free(r);
		}
		if (verbose == 1) {
			if (dbtype == GEOIP_ISP_EDITION) {
				GeoIP_printf(f,"testing GEOIP_ISP_EDITION\n");
			}
			if (dbtype == GEOIP_ORG_EDITION) {
				GeoIP_printf(f,"testing GEOIP_ORG_EDITION\n");
			}
		}
	}
	if (lookupresult == 0) {
		GeoIP_delete(gi);
		if (verbose == 1)
			GeoIP_printf(f,"FAIL\n");
		return GEOIP_SANITY_LOOKUP_FAIL;
	}
	GeoIP_delete(gi);
	if (verbose == 1)
		GeoIP_printf(f,"PASS\n");

	/* install GeoIP.dat.test -> GeoIP.dat */
	err = rename(file_path_test, geoipfilename);
	if (err != 0) {
		GeoIP_printf(f,"GeoIP Install error while renaming file\n");
		return GEOIP_RENAME_ERR;
	}

	if (verbose == 1)
		GeoIP_printf(f,"Done\n");
	free(geoipfilename);
	return 0;
}
