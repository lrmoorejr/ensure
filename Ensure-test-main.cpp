/*
 * Copyright 2026 L. Richard Moore Jr.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

// #define ENSURE_WITH_BACKTRACE
#include "commons/Ensure.hpp"

int main(int, char**) {
	ensure(true, "This should not evaluate {}", 1);
	caution("This ia {}", "caution");
	caution_if(true, "This is another caution");
	caution_if(false, "This is not another caution");
	ensure(false, "This should evaluate to 2: {}", 2);
	return(0);
}