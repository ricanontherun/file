/*

Copyright (C) 2016 Christian Roman

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/

#include "file.h"
#include <string.h>
#include <errno.h>
#include <sys/file.h>
#include <iostream>

File::File() :
    descriptor(0),
    buffer(""),
    advice(ACCESS_ADVICE::SEQUENTIAL),
    mode(O_RDONLY)
{}

File::STATUS File::Open(const char * path) {
    if (access(path, F_OK) == -1) {
        return File::STATUS::NOT_FOUND;
    }

    this->descriptor = open(path, this->mode);

    if ( this->descriptor == -1 ) {
        return File::STATUS::ERROR;
    }

    return this->initialize();
}

File::STATUS File::Open(const std::string & path) {
    return this->Open(path.c_str());
}

File::STATUS File::initialize() {
    int status = fstat(this->descriptor, &(this->file_stat));

    if ( status == -1 ) {
        return File::STATUS::ERROR;
    }

    // Advise the kernel of how the file will be read.
    posix_fadvise(this->descriptor, 0, 0, static_cast<int>(this->advice));

    return File::STATUS::OK;
}

File::~File() {
  if (this->Ok()) {
    close(this->descriptor);
  }
}

/**
 * Was the file opened without error?
 * @return
 */
bool File::Ok() const {
  return this->descriptor > 0;
}

/**
 * Read bytes from the file
 *
 * @param bytes Optional, leave blank for st_blksize bytes.
 * @return
 */
File::READ_STATUS File::Read(ssize_t bytes) {
    bytes = bytes != 0 ? bytes : this->file_stat.st_blksize;
    char buf[bytes];
    ssize_t bytes_read = 0;

    READ_STATUS status = this->ReadIntoBuffer(buf, bytes, &bytes_read);

    if ( status == READ_STATUS::OK ) {
        this->buffer = std::string(buf);
    }

    return status;
}

File::READ_STATUS File::ReadAll() {
    char buf[this->file_stat.st_size + 1];
    char *buf_ptr = buf;
    ssize_t bytes_read = 0;

    do {
        // TODO: this->ReadInfoBuffer((void *) buf, this->file_stat.st_size, &bytes_read);
        bytes_read = read(this->descriptor, buf_ptr, this->file_stat.st_blksize);
        buf_ptr += bytes_read;
    } while (bytes_read > 0);

    if ( bytes_read == 0 )  { // EOF
        this->buffer = std::string(buf);
        return READ_STATUS::END_OF_FILE;
    } else {
        return READ_STATUS::ERROR;
    }
}

void File::Write(const char * buf, size_t len) {

}

/**
 * Return the bytes that were read in the previous Read() operation.
 *
 * @return
 */
const std::string &File::Get() const {
  return this->buffer;
}

File& File::SetReadAdvice(ACCESS_ADVICE advice) {
    this->advice = advice;
    return *this;
}

File& File::SetOpenMode(int mode) {
    this->mode = mode;
    return *this;
}

inline File::READ_STATUS File::ReadIntoBuffer(const char * buffer, size_t bytes_to_read, ssize_t * bytes_read) {
    *bytes_read = 0;

    if ( flock(this->descriptor, LOCK_EX) == -1 ) {
        return READ_STATUS::ERROR;
    }

    lseek(this->descriptor, 0, SEEK_SET);

    ssize_t num_bytes_read = 0;

    do {
        // We can't read past the buffer...we need to know how many are left.
        ssize_t read_size = this->file_stat.st_blksize < bytes_to_read ? this->file_stat.st_blksize : bytes_to_read;
        num_bytes_read = read(this->descriptor, (void *) buffer, read_size);

        if (num_bytes_read <= 0) {
            break;
        }

        *bytes_read += num_bytes_read;
        buffer += num_bytes_read;
        bytes_to_read -= num_bytes_read;
    } while (true);

    if ( flock(this->descriptor, LOCK_UN) == -1 ) {
        return READ_STATUS::ERROR;
    }

    if ( num_bytes_read == -1 ) {
        return READ_STATUS::ERROR;
    }

    return READ_STATUS::OK;
}
