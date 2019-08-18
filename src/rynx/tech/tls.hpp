
#pragma once

#include <rynx/tech/object_storage.hpp>

namespace rynx {
	namespace tls {
		thread_local static rynx::object_storage* tls_storage = nullptr;
		thread_local static int tls_useCount = 0;

		static void init() {
			if (++tls_useCount == 1) {
				tls_storage = new rynx::object_storage();
			}
		}

		static void deinit() {
			if (--tls_useCount == 0) {
				delete tls_storage;
				tls_storage = nullptr;
			}
		}

		struct scoped_tls_used {
			scoped_tls_used() { rynx::tls::init(); }
			~scoped_tls_used() { rynx::tls::deinit(); }
		};

		template<typename T> static void set(T* t) { tls_storage->template set_and_release<T>(t); }
		template<typename T> static T* get() { tls_storage->template get<T>(); }
	}
}