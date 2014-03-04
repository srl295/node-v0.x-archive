// Copyright 2013 the V8 project authors. All rights reserved.
// Copyright (C) 2005, 2006, 2007, 2008, 2009 Apple Inc. All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
// 1.  Redistributions of source code must retain the above copyright
//     notice, this list of conditions and the following disclaimer.
// 2.  Redistributions in binary form must reproduce the above copyright
//     notice, this list of conditions and the following disclaimer in the
//     documentation and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS'' AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS BE LIABLE FOR ANY
// DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
// ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

description(
"This test checks for accuracy in numeric conversions, particularly with large or infinite values."
);

shouldBe("Number(1152921504606847105).toString()", "'1152921504606847200'");
shouldBe("parseInt('1152921504606847105').toString()", "'1152921504606847200'");
shouldBe("(- (- '1152921504606847105')).toString()", "'1152921504606847200'");

shouldBe("Number(0x1000000000000081).toString(16)", "'1000000000000100'");
shouldBe("parseInt('0x1000000000000081', 16).toString(16)", "'1000000000000100'");
shouldBe("(- (- '0x1000000000000081')).toString(16)", "'1000000000000100'");

shouldBe("Number(0100000000000000000201).toString(8)", "'100000000000000000400'");
shouldBe("parseInt('100000000000000000201', 8).toString(8)", "'100000000000000000400'");

shouldBe("(- 'infinity').toString()", "'NaN'");

shouldBe("parseInt('1000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000').toString()", "'Infinity'");
shouldBe("parseInt('0x100000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000', 16).toString()", "'Infinity'");
shouldBe("parseInt('100000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000', 8).toString()", "'Infinity'");

shouldBe("parseInt('9007199254740992e2000').toString()", "'9007199254740992'");
shouldBe("parseInt('9007199254740992.0e2000').toString()", "'9007199254740992'");

shouldBe("parseInt(NaN)", "NaN");
shouldBe("parseInt(-Infinity)", "NaN");
shouldBe("parseInt(Infinity)", "NaN");

shouldBe("parseInt(-0.6).toString()", "'0'");
