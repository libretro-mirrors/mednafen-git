// TODO/WIP
//
// character set is going to be a bit of an issue if we want to allow for encodings other than UTF-8...
//

#ifndef __MDFN_STREAMSPACE_H
#define __MDFN_STREAMSPACE_H


// FileStreamSpace();
// ZIPStreamSpace(const char *path);
class StreamSpace
{
 public:

 enum
 {
  MODE_READ = 0,
  MODE_WRITE,
  MODE_WRITE_SAFE       // Will throw an exception instead of overwriting an existing file.
 };

 StreamSpace();
 virtual ~StreamSpace();


 virtual Stream* open(const char *path, const int mode, const char *purpose = NULL) = 0;

 virtual std::string encoding(void);	// Charset/character encoding for names.

 virtual void chdir(const char *path) = 0;

 virtual void mkdir(const char *path, bool private = false) = 0;

 virtual bool exists(const char *path) = 0;

 virtual void rewind_list(void) = 0;


	name
 	type(directory, regular file, or other) 
 virtual void read_list(SOMETHING?) = 0;
};




#endif
