#include "HCUBE_Defines.h"

#include "HCUBE_EvaluationSet.h"

namespace HCUBE
{
    void EvaluationSet::run()
    {
#ifndef _DEBUG
        try
#endif
        {
            //Process individuals sequentially
            running=true;

            vector<shared_ptr<NEAT::GeneticIndividual> >::iterator tmpIterator;

            tmpIterator = individualIterator;
            for (int a=0;a<individualCount;a++,tmpIterator++)
            {
                while (!running)
                {
                    boost::xtime xt;
#if BOOST_VERSION > 105000
                    boost::xtime_get(&xt, boost::TIME_UTC_);
#else
                    boost::xtime_get(&xt, boost::TIME_UTC);
#endif
                    xt.sec += 1;
                    boost::thread::sleep(xt); // Sleep for 1 second
                }

                experiment->addIndividualToGroup(*tmpIterator);

                if (experiment->getGroupSize()==experiment->getGroupCapacity())
                {
                    //cout_ << "Processing group...\n";
                    experiment->processGroup(generation);
                    //cout_ << "Done Processing group\n";
                    experiment->clearGroup();
                }
            }

            if (experiment->getGroupSize()>0)
            {
                //Oops, maybe you specified a bad # of threads?
                throw CREATE_LOCATEDEXCEPTION_INFO("Error, individuals were left over after run finished!");
            }

            finished=true;
        }
#ifndef _DEBUG
        catch (string s)
        {
			cout_ << "CAUGHT ERROR AT " << __FILE__ << " : " << __LINE__ << endl;
            CREATE_PAUSE(s);
        }
        catch (const char *s)
        {
			cout_ << "CAUGHT ERROR AT " << __FILE__ << " : " << __LINE__ << endl;
            CREATE_PAUSE(s);
        }
        catch (const std::exception &ex)
        {
			cout_ << "CAUGHT ERROR AT " << __FILE__ << " : " << __LINE__ << endl;
            CREATE_PAUSE(ex.what());
        }
        catch (...)
        {
			cout_ << "CAUGHT ERROR AT " << __FILE__ << " : " << __LINE__ << endl;
            CREATE_PAUSE("An unknown exception has occured!");
        }
#endif
    }

}
