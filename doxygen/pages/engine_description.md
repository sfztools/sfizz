# Global view of the engine

The sfizz engine is basically a "Synth" object that takes an SFZ file in, receives MIDI-type events
and is able to render audio through successive calls to a callback function. This is in line with
the way most audio applications and plugins are working. A high-level overview is presented in the
following diagram.

```
                                       C and C++ API entry point

                                                   |
                                                   |
                                                   |
                                                   |
                                          +--------v-------+
                                          |                |
                +--------------------------     Synth      -----------------------------+
                |                         |                |                            |
                |                         +----------------+                            |
                |                                  |                                    |
       +--------v------+                           |                          +---------v--------+
       |               |                 +---------v---------+                |                  |
       |  Region list  |                 | Common resources  |                |    Voice pool    |
       |               |                 |     and state     |                |                  |
       +---------------+                 | ----------------- |                +------------------+
    Built from the SFZ file              |  File pool        |
                                         |  Envelope pool    |            The voices are the polyphony
 Each region is a semi-passive           |  LFO pool         |            of the synth. They are idle
 description object that can             |  Buffer pool      |            and they get activated by the
 decide whether it is "active"           |  Midi state       |            synth to play a region on a
 or not depending on the chain           |  ...              |            specific event. They are then
 of MIDI events it receives.             +-------------------+            "linked" to the region while
 Once activated, a voice is         There are a number of common          it is played, and reset to
 chosen to play the region until    resources that are needed for         their idle state when they
 it ends naturally or through       all the regions and in parti-         are done playing the region.
 note-offs or off-groups.           cular the voices. This includes
                                    all the (preloaded) files for
                                    the SFZ instrument, but will
                                    include in the future the EG
                                    and LFOs that are needed to
                                    achieve compliance with the
                                    SFZ v2 specification. This will
                                    also include a temporary buffer
                                    holder that voices may share.
                                    A common resource of importance
                                    is the MIDI state: note durations
                                    are needed for some opcodes --
                                    for example rt_decay -- and
                                    triggering velocities too.
```

The Synth, Voices and Regions form the bulk of the code complexity. The rest of the engine is dedicated of
mostly helper classes to enable easy management of floating-point buffers in which the audio data is held,
signal processing and accelerated (SIMD) computations, and abstractions that are specific to the SFZ format
such as envelope generators, curves or LFOs.

# Parsing the SFZ files

The sfz file logic is pretty simple and well defined. The sfzformat.com website contains an extensive documentation
on it. At its core, an SFZ file describes a list of `region` objects on which a certain number of "opcodes" will
apply. Opcodes can determine the sample played, the event conditions that will trigger the sample such as the range
of notes, channels, velocities, the processing to apply on the sample while playing, and many more things. It is
also possible to describe a `group` of regions, as well as exclusive groups that will shut off other regions that may
already be playing. There are also `master` groups, and `global` opcodes and some other types.

All the opcodes are declared within a header, in a pseudo-xml markup language that looks like this
```sfz
<global> volume=6
<control> set_cc4=5
<region> key=36 sample=kick.wav
```
Here we have 3 headers (`global`, `control` and `region`) and each header holds some opcodes. All of these opcodes
have a value---for example the volume is equal to 6 in the `global` header. Some opcodes also have parameters.
The `control` header holds an opcode `set_cc` with the parameter `4` and value `5`. The parameter here is the CC to set,
and the value at which to set it is 5.

The parsing logic of sfizz is handled through a base class called Parser---a very original choice. This parser has
a virtual callback that gets called whenever a header description is "complete", along with a list of opcodes that
apply to the header. Subclassing the Parser then allows to build different SFZ handlers, from full-blown synths as
with sfizz to simpler things such as printers (see in particular https://github.com/sfztools/sfz-flat/). If we look
at the core of the latter example, it will look something like the following

```cpp
class PrintingParser: public sfz::Parser
{
protected:
    void callback(absl::string_view header, const std::vector<sfz::Opcode>& members) final
    {
        switch (hash(header)) // The hash(...) function transforms strings to large integers
        {
        case hash("global"): // It is also compile-time defined, which allows to do switch-case
                             // statements on strings, something that is usually not possible
            globalMembers = members; // We save the global headers since they apply to the next
                                     // region (and groups and masters)
            masterMembers.clear();
            groupMembers.clear();
            break;
        case hash("master"):
            masterMembers = members; // So on
            groupMembers.clear();
            break;
        case hash("group"):
            groupMembers = members; // .. and so forth
            break;
        case hash("region"):
            std::cout << "<" << header << ">" << ' '; // Now we print the region along with all the opcodes
                                                      // we memorized from earlier headers.
            printMembers(globalMembers);
            printMembers(masterMembers);
            printMembers(groupMembers);
            printMembers(members);
            std::cout << '\n';
            break;
        default:
            std::cout << "<" << header << ">" << ' ';
            printMembers(members);
            std::cout << '\n';
            break;
        }
    }
private:
    std::vector<sfz::Opcode> globalMembers;
    std::vector<sfz::Opcode> masterMembers;
    std::vector<sfz::Opcode> groupMembers;
    void printMembers(const std::vector<sfz::Opcode>& members)
    {
        for (auto& member: members)
        {
            std::cout << member.opcode;
            if (member.parameter)
                std::cout << +*member.parameter;
            std::cout << "=" << member.value;
            std::cout << ' ';
        }
    }
};
```

The main function is then quite straightforward and we call a function from the Parser class that loads a file
```cpp
int main(int argc, char** argv)
{
    PrintingParser parser;
    parser.loadSfzFile("my_sfz_file.sfz");
    return 0;
}
```
If you circle back to the parser you will see that opcodes are stored in an `Opcode` class. This class does some parsing
itself and separates the opcode name itself, parameters if any, and the value. Opcodes are very cheap to copy and pass
around because they only refer to characters in the file that are stored inside the `Parser` class, so feel free to
create vectors of them and move them around.

Note that you may also derive the loadSfzFile method if you have any processing you need to do before the actual parsing happens.

# Building the region list in sfizz

The callback method from sfizz is actually quite similar to the one shown above, except that instead of printing the region
we actually fill a big structure from it.
