#ifndef __src__broadcast__consistent__consistent__verifier__hpp
#define __src__broadcast__consistent__consistent__verifier__hpp


namespace psb
{
    using namespace drop;

    // verifier

    // Public static members

    template <typename type> class consistent <type> :: verifier :: system consistent <type> :: verifier :: system;

    // Constructors

    template <typename type> consistent <type> :: verifier :: verifier() : _alive(true), _thread(&verifier :: run, this)
    {
    };

    // Destructor

    template <typename type> consistent <type> :: verifier :: ~verifier()
    {
        this->_guard([&]()
        {
            this->_alive = false;
        });

        this->_semaphore.post();
        this->_thread.join();
    }

    // Methods

    template <typename type> promise <std :: vector <uint32_t>> consistent <type> :: verifier :: verify(const typename broadcast <type> :: batch & batch)
    {
        workunit workunit{.batch = batch};

        this->_guard([&]()
        {
            this->_workunits.push_back(workunit);
        });

        this->_semaphore.post();
        return workunit.promise;
    }

    // Services

    template <typename type> void consistent <type> :: verifier :: run()
    {
        while(this->_guard([&](){return this->_alive;}))
        {
            auto workunit = this->_guard([&]() -> optional <struct workunit>
            {
                if(this->_workuinits.size())
                {
                    struct workunit workunit = this->_workunits.front();
                    this->_workunits.pop_front();
                    return workunit;
                }
                else
                    return optional <struct workunit> ();
            });

            if(workunit)
            {
                std :: vector <uint32_t> tampered;
                uint32_t sequence = 0;

                for(const auto & block : workunit.batch.blocks)
                    for(const auto & message : block)
                    {
                        try
                        {
                            verifier verifier(message.feed);
                            verifier.verify(message.signature, message.sequence, message.payload);
                        }
                        catch(...)
                        {
                            tampered.push_back(sequence);
                        }

                        sequence++;
                    }

                workunit.promise.resolve(tampered);
            }

            this->_semaphore.wait();
        }
    }

    // system

    // Static members

    template <typename type> thread_local size_t consistent <type> :: verifier :: system :: roundrobin = 0;

    // Private constructors

    template <typename type> consistent <type> :: verifier :: system :: system() : _verifiers(new verifier[std :: thread :: hardware_concurrency()]), _size(std :: thread :: hardware_concurrency())
    {
    }

    // Destructor

    template <typename type> consistent <type> :: verifier :: system :: ~system()
    {
        delete [] this->_verifiers;
    }

    // Methods

    template <typename type> typename consistent <type> :: verifier & consistent <type> :: verifier :: system :: get()
    {
        return this->_verifiers[(roundrobin++) % this->_size];
    }
}

#endif
