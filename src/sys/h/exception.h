#ifndef _PUNIX_EXCEPTION_H_
#define _PUNIX_EXCEPTION_H_

/* exception stack frames */

union exception_info {

	struct {
		short function_code;
		void *access_address;
		short instruction_register;
		short status_register;
		void *program_counter;
	} address_error, bus_error;

	struct {
		short status_register;
		void *program_counter;
		short vector_offset;
		void *address;
	} other;

};

#endif
