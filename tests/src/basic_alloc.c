/*
 * Copyright 2021 Garrett Fairburn <breadboardfox@gmail.com>
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
#include "test_prelude.h"

TestCase {
    void *clos0 = NULL;

    AssertBoolEqual(CClosureCheck(clos0), false);
    clos0 = CClosureNew(NULL, NULL, false);
    AssertBoolEqual(CClosureCheck(clos0), true);
    CClosureFree(clos0);
    AssertBoolEqual(CClosureCheck(clos0), false);

    Pass();
}