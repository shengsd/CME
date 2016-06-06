#ifndef MDP_PARSER_H
#define MDP_PARSER_H

#ifdef _MSC_VER
#pragma warning( disable : 4503 4355 4786 4290 )
#endif
#include "ExMDP.h"
#include "otf_api/Listener.h"
#include "otf_api/IrCollection.h"
#include <fstream>


namespace MDP
{
	class FieldMap : public MDPFieldMap
	{
	public:
		typedef std::map<int, Field> Fields;
		typedef std::map<int, std::vector < FieldMap* >> Groups;

		~FieldMap()
		{
			clear();
		}

		void setField(int tag, const Field& field)
		{
			Fields::iterator i = m_fields.find( tag );
			if( i == m_fields.end() )
				m_fields.insert( Fields::value_type( tag, field ) );
		}

		Field* getField(int tag)
		{
			Fields::iterator i = m_fields.find( tag );
			if ( i == m_fields.end() )
				return NULL;
			else
				return &(i->second);
		}


		bool isSetGroup(int tag)
		{
			Groups::iterator i = m_groups.find( tag );
			if ( i == m_groups.end() )
				return false;
			else
				return true;		
		}

		int getFieldMapNumInGroup(int tag)
		{
			Groups::iterator i = m_groups.find( tag );
			if ( i == m_groups.end() )
				return 0;
			else
				return i->second.size();		
		}



		void setGroup(int tag, int num)
		{
			Groups::iterator i = m_groups.find( tag );
			if ( i == m_groups.end())
			{
				std::vector < FieldMap* > vpFieldMap;
				while (num)
				{
					FieldMap* pFieldMap = new FieldMap;
					vpFieldMap.push_back( pFieldMap );
					--num;
				}
				m_groups.insert( Groups::value_type( tag, vpFieldMap ) );
			}
		}

		FieldMap* getFieldMapPtrInGroup(int tag, int index)
		{
			Groups::iterator i = m_groups.find( tag );
			if ( i != m_groups.end() )
			{
				return i->second[index];
			}
			else
			{
				return NULL;
			}		
		}

		void clear()
		{
			m_fields.clear();
			Groups::iterator i;
			for ( i = m_groups.begin(); i != m_groups.end(); ++i )
			{
				std::vector<FieldMap* > ::iterator j;
				for ( j = i->second.begin(); j != i->second.end(); ++j )
				{
					delete *j;
				}
			}
			m_groups.clear();		
		}

	private:
		Fields m_fields;
		Groups m_groups;
	};

	// class to encapsulate Ir repository as well as Ir callback for dispatch
	class IrRepo : public IrCollection, public Ir::Callback
	{
	public:
		// save a reference to the Listener so we can print out the offset
		//IrRepo(Listener &listener) : listener_(listener) {};

		virtual Ir *irForTemplateId(const int templateId, const int schemaVersion)
		{
			//std::cout << "Message lookup id=" << templateId << " version=" << schemaVersion << " offset " << /*listener_.bufferOffset() << */std::endl;

			// lookup in IrCollection the IR for the template ID and version
			return (Ir *)IrCollection::message(templateId, schemaVersion);
		};

		// 	 	private:
		// 	 	Listener &listener_;
	};



	// class to encapsulate all the callbacks
	class CarCallbacks : public OnNext, public OnError, public OnCompleted
	{
	private:
		Listener &listener_;
		int indent_;
		bool m_status;
		FieldMap* m_pFieldMap;
		FieldMap m_fieldMap;
		std::ofstream* m_fstr;

	public:
		// save reference to listener for printing offset
		CarCallbacks(Listener &listener, std::ofstream* pFstr)
			:listener_(listener),
			indent_(0),
			m_status(false),
			m_fstr(pFstr)
		{
			m_pFieldMap = &m_fieldMap;
		}
		/*
		CarCallbacks(Listener &listener)
		:listener_(listener),
		indent_(0),
		m_status(false)
		{
		m_pFieldMap = &m_fieldMap;
		}
		*/

		~CarCallbacks()
		{
		}

		// get decoding status
		bool getStatus()
		{
			return m_status;
		}

		// callback for when a field is encountered
		virtual int onNext(const Field &f)
		{
			int tag = f.schemaId();
			m_pFieldMap->setField(tag, f);
			if (m_fstr)
			{


				*m_fstr << "Field name=\"" << f.fieldName() << "\" id=" << f.schemaId();

				if (f.isComposite())
				{
					*m_fstr << ", composite name=\"" << f.compositeName() << "\"";
				}

				*m_fstr << std::endl;

				if (f.isEnum())
				{
					*m_fstr << " Enum [" << f.validValue() << "]";
					printEncoding(f, 0); // print the encoding. Index is 0.
				}
				else if (f.isSet())
				{

					*m_fstr << " Set ";
					// print the various names for the bits that are set
					for (std::vector<std::string>::iterator it = ((std::vector<std::string>&)f.choices()).begin(); it != f.choices().end(); ++it)
					{

						*m_fstr << "[" << *it << "]";
					}

					printEncoding(f, 0); // print the encoding. Index is 0.
				}
				else if (f.isVariableData())
				{
					// index 0 is the length field type, value, etc.
					// index 1 is the actual variable length data


					*m_fstr << " Variable Data length=" << f.length(1);

					char tmp[256];
					f.getArray(1, tmp, 0, f.length(1));  // copy the data
					*m_fstr << " value=\"" << std::string(tmp, f.length(1)) << "\"";

					*m_fstr << " presence=" << presenceStr(f.presence(1));
					// printing out meta attributes
					//m_fstr << " epoch=\"" << f.getMetaAttribute(Field::EPOCH, 1) << "\"";
					//m_fstr << " timeUnit=\"" << f.getMetaAttribute(Field::TIME_UNIT, 1) << "\"";
					//m_fstr << " semanticType=\"" << f.getMetaAttribute(Field::SEMANTIC_TYPE, 1) << "\"";
					*m_fstr << std::endl;

				}
				else // if not enum, set, or var data, then just normal encodings, but could be composite
				{
					for (int i = 0, size = f.numEncodings(); i < size; i++)
					{
						printEncoding(f, i);
					}
				}
			}

			return 0;
		};

		// callback for when a group is encountered
		virtual int onNext(const Group &g)
		{
			int tag = g.schemaId();
			m_pFieldMap->setGroup(tag, g.numInGroup());

			// group started
			if (g.event() == Group::START)
			{
				m_pFieldMap = m_pFieldMap->getFieldMapPtrInGroup(tag, g.iteration());

				if (m_fstr)
				{
					*m_fstr << "Group name=\"" << g.name() << "\" id=\"" << g.schemaId() << "\" start (";
					*m_fstr << g.iteration() << "/" << g.numInGroup() - 1 << "):" << "\n";
				}

				if (g.iteration() == 1)
				{
					indent_++;
				}
			}
			else if (g.event() == Group::END)  // group ended
			{
				m_pFieldMap = &m_fieldMap;

				if (m_fstr)
				{
					*m_fstr << "Group name=\"" << g.name() << "\" id=\"" << g.schemaId() << "\" end (";
					*m_fstr << g.iteration() << "/" << g.numInGroup() - 1 << "):" << "\n";
				}

				if (g.iteration() == g.numInGroup() - 1)
				{
					indent_--;
				}
			}
			return 0;
		};

		// callback for when an error is encountered
		virtual int onError(const Error &e)
		{
			if (m_fstr)
				*m_fstr << "Error " << e.message() << " at offset " << listener_.bufferOffset() << "\n";
			m_status = false;
			return 0;
		};

		// callback for when decoding is completed
		virtual int onCompleted()
		{
			if (m_fstr)
				*m_fstr << "Completed" << "\n";
			m_status = true;
			return 0;
		};

		FieldMap& getFieldMapRef()
		{
			return m_fieldMap;
		}

		FieldMap* getFieldMapPtr()
		{
			return &m_fieldMap;
		}

	protected:

		// print out details of an encoding
		void printEncoding(const Field &f, int index)
		{
			if (m_fstr)
			{

				*m_fstr << " name=\"" << f.encodingName(index) << "\" length=" << f.length(index);
				switch (f.primitiveType(index))
				{
				case Ir::CHAR:
					if (f.length(index) == 1)
					{
						*m_fstr << " type=CHAR value=\"" << (char)f.getUInt(index) << "\"";

					}
					else
					{
						char tmp[1024];

						// copy data to temp array and print it out.
						f.getArray(index, tmp, 0, f.length(index));
						*m_fstr << " type=CHAR value=\"" << std::string(tmp, f.length(index)) << "\"";
					}
					break;
				case Ir::INT8:
					*m_fstr << " type=INT8 value=\"" << f.getInt(index) << "\"";
					break;
				case Ir::INT16:
					*m_fstr << " type=INT16 value=\"" << f.getInt(index) << "\"";
					break;
				case Ir::INT32:
					if (f.length() == 1)
					{
						*m_fstr << " type=INT32 value=\"" << f.getInt(index) << "\"";
					}
					else
					{
						char tmp[1024];

						// copy data to temp array and print it out.
						f.getArray(index, tmp, 0, f.length(index));
						*m_fstr << " type=INT32 value=";
						for (int i = 0, size = f.length(index); i < size; i++)
						{
							*m_fstr << "{" << *((int32_t *)(tmp + (sizeof(int32_t) * i))) << "}";
						}
					}
					break;
				case Ir::INT64:
					*m_fstr << " type=INT64 value=\"" << f.getInt(index) << "\"";
					break;
				case Ir::UINT8:
					*m_fstr << " type=UINT8 value=\"" << f.getUInt(index) << "\"";
					break;
				case Ir::UINT16:
					*m_fstr << " type=UINT16 value=\"" << f.getUInt(index) << "\"";
					break;
				case Ir::UINT32:
					*m_fstr << " type=UINT32 value=\"" << f.getUInt(index) << "\"";
					break;
				case Ir::UINT64:
					*m_fstr << " type=UINT64 value=\"" << f.getUInt(index) << "\"";
					break;
				case Ir::FLOAT:
					*m_fstr << " type=FLOAT value=\"" << f.getDouble(index) << "\"";
					break;
				case Ir::DOUBLE:
					*m_fstr << " type=DOUBLE value=\"" << f.getDouble(index) << "\"";
					break;
				default:
					break;
				}
				*m_fstr << " presence=" << presenceStr(f.presence(index));
				// printing out meta attributes for encodings
				//*m_fstr << " epoch=\"" << f.getMetaAttribute(Field::EPOCH, index) << "\"";
				//*m_fstr << " timeUnit=\"" << f.getMetaAttribute(Field::TIME_UNIT, index) << "\"";
				//*m_fstr << " semanticType=\"" << f.getMetaAttribute(Field::SEMANTIC_TYPE, index) << "\"";
				*m_fstr << std::endl;
			}
		}

		// print presence
		const char *presenceStr(Ir::TokenPresence presence)
		{
			switch (presence)
			{
			case Ir::SBE_REQUIRED:
				return "REQUIRED";
				break;

			case Ir::SBE_OPTIONAL:
				return "OPTIONAL";
				break;

			case Ir::SBE_CONSTANT:
				return "CONSTANT";
				break;

			default:
				return "UNKNOWN";
				break;

			}
		}

	};

}

#endif
