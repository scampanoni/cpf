#include "liberty/Talkdown/AnnotationParser.h"

namespace AutoMP
{
  // Note that this does not populate the "loop" field of Annotation
  AnnotationSet parseAnnotationsForInst(llvm::Instruction *i)
  {
    using namespace llvm;
    AnnotationSet annots;

    MDNode *meta = i->getMetadata("note.noelle");
    if ( meta )
    {
      auto operands = meta->operands();
      for ( auto &op : operands )
      {
        MDNode *casted_meta = dyn_cast<MDNode>(op.get());
        assert( casted_meta && "Couldn't cast operand to MDNode" );

        MDString *key = dyn_cast<MDString>(casted_meta->getOperand(0));
        MDString *value = dyn_cast<MDString>(casted_meta->getOperand(1));
        assert( key && value && "Couldn't cast key or value from annotation" );

        // don't care about the loop right now...
        annots.emplace(nullptr, key->getString(), value->getString());
      }
    }

    return annots;
  }
};