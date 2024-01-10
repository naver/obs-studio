#pragma once

namespace vst {
static QString getFileName(const QString &path)
{
	if (path.isEmpty()) {
		return {};
	} else if (int off1 = path.lastIndexOf('/'); off1 >= 0) {
		return path.mid(off1 + 1);
	} else if (int off2 = path.lastIndexOf('\\'); off2 >= 0) {
		return path.mid(off2 + 1);
	}
	return path;
}
}

typedef void *(*process_create)(uint32_t process_id);
typedef void (*process_destroy)(void *process);
typedef uint32_t (*current_process_id)();
typedef bool (*process_terminate)(void *process, int exit_code);
