This is a simple skeleton of an A/UX device driver. It's probably the first new A/UX device driver that's been written since the turn of the millenium, if not longer.

# What

There is a list, curated by @mietek, of [all known A/UX related material and official software](https://gist.github.com/mietek/174b27e879a7b83d502a351ea3aaa831). For material that has been located and preserved, it includes links to said material. This list, the material on it, and the mysterious wizard who manages it all are very helpful to anyone trying to do anything with A/UX in the modern age. But, conspicuously unpreserved are any material relating to writing new drivers for the greatest and most advanced operating system that a decked-out Quadra 950 can run. (Well, second or third greatest, at least.).

In particular:

- [M8037/B] A/UX Device Drivers Kit 1.1 / Building A/UX Device Drivers
- [M8037/C] A/UX Device Drivers Kit 2.0 / Building A/UX Device Drivers

Either of those would be very nice to have. Very, very nice. But we don't have them, and after 20+ years it looks like we never will.

Sigh.

Anyway, thanks to @mietek stalking [Paul Campbell](http://taniwha.com/~paul/index.html) on Hacker News, and the fact that the source code to A/UX 0.7 has been released, we all managed to pull together and make a new device driver that gets loaded and executed and behaves semi-adequately.

Which is this!

# How

There are three parts to making a working A/UX device driver.

1. The driver itself.
2. The master file, which goes in `/etc/master.d`. This tells `autoconfig(1M)` about our driver.
3. The init file, which goes in `/etc/install.d/init.d`, and which `autoconfig(1M)` will then install in `/etc/init.d` and run. This creates our device node(s). 

All the information you need for 2 is in `master(4)`. Cargo cult copy-pasting will get you a usable 3. Which leaves the driver.

# The driver

The driver file is an m68k COFF object file. The only compilers I was able to get my hands on that can output this are the stock pre-ANSI C compiler that came with A/UX and the GCC port that's on jagubox. I used the former for this test driver, so there would be as few moving parts as possible.

A character driver for A/UX contains a few functions. They are described here with their bare names, but you should provide a prefix for them. The test driver uses the prefix `test_`, so `init` becomes `test_init`, etc.

## `init`

Called during kernel init, (hopefully) before your device is opened. No parameters, return value isn't used.

## `open`

Called when a process opens your device. Return 0 for success, or some errno value for a failure. The sole parameter is a `dev_t` indicating which device node was actually opened. For the sake of your sanity, use `minor()` from `<sys/sysmacros.h>` to extract the minor number from this and don't try to use the major number for anything. (After all, you have no way to know what major number `autoconfig` assigned you. You *are* letting `autoconfig` assign you a guaranteed non-conflicting major number, right?)

## `close`

Called when a process closes your device. Again you get a `dev_t`, again you should only use it with `minor()`.

## `read`/`write`

Called when a process reads/writes an open device. First argument is a `dev_t`, same drill as above. Second parameter is a `struct uio*`. For your own sanity, you should only use this with `uiomove`. What header is `uiomove` in? Well, it's certainly not `<sys/uio.h>`, but you definitely want to include it because it contains definitions for `struct uio`, `UIO_READ`, and `UIO_WRITE`. Good news: It's not in any header, because this is K&R C and prototypes won't be cool until well into the 90's!

Ahem.

`uiomove` moves bytes between kernel space and userspace. It has all kinds of complicated vectored IO and address space mapping support that I couldn't grok after five seconds of looking at the header. Fortunately, you don't have to worry about any of that. To implement `read`, all you have to do is call `uiomove(buf, num_bytes, UIO_READ, uio)` to copy that many bytes out of your `buf` and into wherever the caller wanted to put them. For `write`, you call `uiomove(buf, num_bytes, UIO_WRITE, uio)` and it will copy that many bytes into your `buf`, ready to be used in whatever nefarious scheme you have planned for them. `uiomove` will return 0 for success and an error code on error, and that's exactly what you need to return, which means you can return `uiomove`'s return value directly.

How do you know how many bytes to copy out? I'm not sure. There's probably some `uio` function that does that. If you have a simple device driver that has a fixed-size record (like testc's one-byte "records"), you don't have to worry about it. If you try to write/read too many bytes, `uiomove` will return an error. If you try to write/read too few, the number that you actually wrote/read will be reported to userspace just fine. `uiomove` really is doing all the heavy lifting here.

## `select`

Fly in the ointment.

First parameter is a `dev_t`, second parameter is an `int`. It will have the value 0 if this is an exception select(?), 1 if this is a read select, 2 if this is a write select. You return 1 if there is data available to read (1) or room available to write (2). But if you're not ready, things get interesting.

In `<sys/user.h>` is a global, `u`. In that global is a field, `u_procp`. If you save the value of this, and return 0, you can later pass that saved `u_procp` value as the first parameter to `selwakeup` (where the second parameter is some flag, 0 works), then the process that was `select`ing on you will wake up.

If you don't want to implement wakeups, just implement a stub `select` function that always returns 1. This will prevent non-blocking IO with your device from being possible. Depending on what kind of device it is, that may be just fine.

(What should you do if there's an exception select? I don't know. To be honest, I'm not even sure what it'd be used for. `testc` just returns 0 in that case, but always returning 1 instead is probably safer.)

## `ioctl`

Include `<sys/ioctl.h>`. Define `ioctl` request numbers with `_IOR` if the request is like a read, `_IOW` if it's like a write, `_IOWR` if it's like both. (It would have been `_IORW` but "stdio got there first".) First parameter is (usually) a character constant, second parameter is a small ID number, combined they form a unique ioctl request number. Third parameter is the type being read/written, often an `int`, sometimes a `struct`.

Your `ioctl` function will receive four parameters. First is a `dev_t`. Second is the ioctl request number. Third is a pointer to the type you gave as the third parameter to `_IO*`. Fourth is "arg", I honestly don't know what it's for. You do the read/write/action/whatever, return 0 for success or some errno code for failure. If the command is unrecognized, return `EINVAL`.

It's possible to make ioctl requests accept arbitrarily-long inputs and produce arbitrarily-long outputs, but you need to take greater care in defining (and decoding!?) your ioctl numbers, and... that's starting to sound a lot like something you should be implementing in terms of `read` or `write` instead.

## Slot identification, interrupts, making simple non-blocking IO work...

¯\\_(ツ)_/¯

This test driver requires [my paravirtualized framebuffer](https://github.com/SolraBizna/mac_qfb_driver) to be in NuBus slot C. It will only be picked up by `autoconfig` if the card is installed, and it will only be able to do its debug output if the card is in slot `C` (otherwise it'll bus error). There are ways to receive slot interrupts, and to find out which slots exist and are your card, but I haven't gone digging for them yet so I don't know what they are.

As for how to make `write()` or `read()` block if no data is available, I'm sure it's possible (since ttys do it) but I haven't dug for that either.

# Legalese

This repository is licensed under the something-or-other license. I don't really care what you do with it, as long as you don't end up somehow using it for genocide or war crimes or something.

Okay, I lied, I do care what you do with it. If you use this, and end up making a driver that's actually useful, I'd like to know about it because that sounds really cool.
