/*
 * UploadHandler.cpp
 *
 *  Created on: Mar 22, 2017
 *      Author: erik
 */

#include "UploadHandler.hpp"

#define MD5_STATIC static
#include "md5.inl"

struct tfile_checksum {
	char name[128];
	unsigned long long length;
	md5_state_t chksum;
};

#define MAX_FILES (10)

struct tfiles_checksums {
	int index;
	struct tfile_checksum file[MAX_FILES];
};

int
field_disp_read_on_the_fly(const char *key,
                           const char *filename,
                           char *path,
                           size_t pathlen,
                           void *user_data)
{
	struct tfiles_checksums *context = (struct tfiles_checksums *)user_data;

	snprintf(path, pathlen, "/home/erik/resources/%s", filename);

	fprintf(stderr,"Path is %s\n",path);
	if (context->index < MAX_FILES) {
		context->index++;
		strncpy(context->file[context->index - 1].name, filename, 128);
		context->file[context->index - 1].name[127] = 0;
		context->file[context->index - 1].length = 0;
		md5_init(&(context->file[context->index - 1].chksum));
		return FORM_FIELD_STORAGE_STORE;
	}
	return FORM_FIELD_STORAGE_ABORT;
}

int
field_stored(const char *path, long long file_size, void *user_data)
{
	struct mg_connection *conn = (struct mg_connection *)user_data;

	mg_printf(conn,
	          "stored as %s (%lu bytes)\r\n\r\n",
	          path,
	          (unsigned long)file_size);

	return 0;
}


int
field_get_checksum(const char *key,
                   const char *value,
                   size_t valuelen,
                   void *user_data)
{

	struct tfiles_checksums *context = (struct tfiles_checksums *)user_data;
	(void)key;

	context->file[context->index - 1].length += valuelen;
	md5_append(&(context->file[context->index - 1].chksum),
	           (const md5_byte_t *)value,
	           valuelen);

	return 0;
}


bool UploadHandler::handleGet(CivetServer *server, struct mg_connection *conn)
	{
		return handleAll("GET", server, conn);
	}

bool UploadHandler::handlePost(CivetServer *server, struct mg_connection *conn)
	{
		return handleAll("POST", server, conn);
	}

bool UploadHandler::handleAll(const char *method,
          CivetServer *server,
          struct mg_connection *conn)
{
	std::string s[8] = "";
	std::string dummy;
	std::string param = "chan";

	/* upload was submitted */
	if(method == "POST")
	{
		/* Handler may access the request info using mg_get_request_info */
		const struct mg_request_info *req_info = mg_get_request_info(conn);
		int i, j, ret;
		struct tfiles_checksums chksums;
		md5_byte_t digest[16];
		struct mg_form_data_handler fdh = {field_disp_read_on_the_fly,
		                                   field_get_checksum,
										   field_stored,
		                                   (void *)&chksums};

			/* It would be possible to check the request info here before calling
			 * mg_handle_form_request. */
			(void)req_info;

			memset(&chksums, 0, sizeof(chksums));

			mg_printf(conn,
			          "HTTP/1.1 200 OK\r\n"
			          "Content-Type: text/html\r\n"
			          "Connection: close\r\n\r\n");

			/* Call the form handler */
			mg_printf(conn, "<html><head><meta http-equiv=\"refresh\" content=\"3;url=/upload\" /></head><body>");
			mg_printf(conn, "<h2>Uploaded files and their checksums:</h2>");
			ret = mg_handle_form_request(conn, &fdh);
			for (i = 0; i < chksums.index; i++) {
				md5_finish(&(chksums.file[i].chksum), digest);
				/* Visual Studio 2010+ support llu */
				mg_printf(conn,
				          "\r\n%s       ",
				          chksums.file[i].name);
				for (j = 0; j < 16; j++) {
					mg_printf(conn, "%02x", (unsigned int)digest[j]);
				}
				mg_printf(conn,"</br>");
			}
			mg_printf(conn, "\r\n%i files\r\n", ret);
			mg_printf(conn, "</body></html>");
	}
	else
	{
		mg_printf(conn,
			          "HTTP/1.1 200 OK\r\nContent-Type: "
			          "text/html\r\nConnection: close\r\n\r\n");
		mg_printf(conn, "<html><head><meta charset=\"UTF-8\"></head><body>");
		mg_printf(conn, "<form action=\"/upload\" method=\"POST\" enctype=\"multipart/form-data\">");
		std::stringstream ss;
	    ss << "<h2>File upload. Maximaal " << MAX_FILES << " bestanden per keer. </h2>";
	    ss << "<input type=\"file\" name=\"file\" id=\"file\" multiple>";
	    ss <<  "</br>";
	    ss << "<input type=\"submit\" value=\"Upload...\" id=\"submit\">";
	    ss <<  "</br>";
	    ss << "<a href=\"/\">Home</a>";
	    mg_printf(conn, ss.str().c_str());
		mg_printf(conn, "</body></html>");
	}

	return true;
}

UploadHandler::UploadHandler() {
}

UploadHandler::~UploadHandler() {
	// TODO Auto-generated destructor stub
}

