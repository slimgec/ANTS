#ifndef itkantsReadWriteTransform_h
#define itkantsReadWriteTransform_h

#include "itkDisplacementFieldTransform.h"
#include "itkImageFileReader.h"
#include "itkImageFileWriter.h"
#include "itkTransformFileReader.h"
#include "itkTransformFileWriter.h"

#include "itkCompositeTransform.h"

namespace itk
{
namespace ants
{
template <unsigned VImageDimension>
typename itk::Transform<double, VImageDimension, VImageDimension>::Pointer
ReadTransform(const std::string &filename)
{
  //We must explicitly check for file existance because failed reading is an acceptable
  //state for non-displacment feilds.
  if( ! itksys::SystemTools::FileExists( filename.c_str() ) )
    {
    std::cerr << "Transform file does not exist: " << filename << std::endl;
    return NULL;
    }

  bool hasTransformBeenRead = false;

  typedef typename itk::DisplacementFieldTransform<double, VImageDimension>   DisplacementFieldTransformType;
  typedef typename DisplacementFieldTransformType::DisplacementFieldType DisplacementFieldType;
  typedef itk::ImageFileReader<DisplacementFieldType> DisplacementFieldReaderType;
  typename DisplacementFieldReaderType::Pointer fieldReader = DisplacementFieldReaderType::New();

  //There are known tranform type extentions that should not be considered as imaging files
  //That would be used as deformatino feilds
  //If file is an hdf5 file, assume it is a tranform instead of an image.
  if ( filename.find(".h5") == std::string::npos
    && filename.find(".hdf5") == std::string::npos
    && filename.find(".hdf4") == std::string::npos
    && filename.find(".mat") == std::string::npos
    && filename.find(".txt") == std::string::npos
    )
    {
    try
      {
      fieldReader->SetFileName( filename.c_str() );
      fieldReader->Update();
      hasTransformBeenRead = true;
      }
    catch( ... )
      {
      hasTransformBeenRead = false;
      }
    }

  typedef typename itk::Transform<double, VImageDimension, VImageDimension> TransformType;
  typename TransformType::Pointer transform;
  if( hasTransformBeenRead )
    {
    typename DisplacementFieldTransformType::Pointer displacementFieldTransform =
      DisplacementFieldTransformType::New();
    displacementFieldTransform->SetDisplacementField( fieldReader->GetOutput() );
    transform = dynamic_cast<TransformType *>( displacementFieldTransform.GetPointer() );
    }
  else
    {
    typename itk::TransformFileReader::Pointer transformReader
      = itk::TransformFileReader::New();

    transformReader->SetFileName( filename.c_str() );
    try
      {
      transformReader->Update();
      }
    catch( const itk::ExceptionObject & e )
      {
      std::cerr << "Transform reader for "
        << filename << " caught an ITK exception:\n";
      e.Print( std::cerr );
      return transform;
      }
    catch( const std::exception & e )
      {
      std::cerr << "Transform reader for "
        << filename << " caught an exception:\n";
      std::cerr << e.what() << std::endl;
      return transform;
      }
    catch( ... )
      {
      std::cerr << "Transform reader for "
        << filename << " caught an unknown exception!!!\n";
      return transform;
      }

    const typename itk::TransformFileReader::TransformListType * const listOfTransforms=transformReader->GetTransformList();
    transform = dynamic_cast<TransformType *>( listOfTransforms->front().GetPointer() );
#if 0 //HACK:  THIS IS FOR DEBUGGING, remove when composite transforms work
    std::cout << "HACK:  " << listOfTransforms->size() << std::endl;
    for( typename itk::TransformFileReader::TransformListType::const_iterator it = listOfTransforms->begin();
      it != listOfTransforms->end(); ++it)
      {
      std::cout << " HACK " << (*it)->GetNameOfClass() << std::endl;
      }
    // When reading a composite tranform file, we need to re-construct it
    // properly.  This should be fixed in ITKv4, but currently is not the case
    static const std::string CompositeTransformID("CompositeTransform");
    if ( listOfTransforms->front()->GetNameOfClass() == CompositeTransformID )
      {
      const typename itk::CompositeTransform<double,VImageDimension>::ConstPointer tempComp=
        dynamic_cast<const itk::CompositeTransform<double,VImageDimension> * >( listOfTransforms->front().GetPointer() );
      std::cout << "Size of TransformQueue is: " << tempComp->GetNumberOfTransforms() << std::endl;
      for (unsigned int i =0; i< tempComp->GetNumberOfTransforms() ; ++i)
        {
        std::cout << i << " transform is of type " << tempComp->GetNthTransform(i)->GetNameOfClass() << std::endl;
        std::cout << i << " FixedParameters      " << tempComp->GetNthTransform(i)->GetFixedParameters() << std::endl;
        }
      }
#endif
    }
  return transform;
}

template <unsigned int VImageDimension>
int
WriteTransform(typename itk::Transform<double,VImageDimension,VImageDimension>::Pointer &xfrm,
               const std::string &filename)
{
  typedef typename itk::DisplacementFieldTransform<double, VImageDimension> DisplacementFieldTransformType;
  typedef typename DisplacementFieldTransformType::DisplacementFieldType    DisplacementFieldType;
  typedef typename itk::ImageFileWriter<DisplacementFieldType>              DisplacementFieldWriter;
  typedef itk::TransformFileWriter                                          TransformWriterType;

  DisplacementFieldTransformType *dispXfrm =
    dynamic_cast<DisplacementFieldTransformType *>(xfrm.GetPointer());

  // if it's a displacement field transform
  try
    {
    if(dispXfrm != 0)
      {
      typename DisplacementFieldType::Pointer dispField =
        dispXfrm->GetDisplacementField();
      typename DisplacementFieldWriter::Pointer writer =
        DisplacementFieldWriter::New();
      writer->SetInput(dispField);
      writer->SetFileName(filename.c_str());
      writer->Update();
      }
    else
      // regular transform
      {
      typename TransformWriterType::Pointer transformWriter = TransformWriterType::New();
      transformWriter->SetInput(xfrm);
      transformWriter->SetFileName(filename.c_str());
      transformWriter->Update();
      }
    }
  catch( itk::ExceptionObject & err )
    {
    std::cerr << "Can't write transform file " << filename << std::endl;
    std::cerr << "Exception Object caught: " << std::endl;
    std::cerr << err << std::endl;
    return EXIT_FAILURE;
    }
  return EXIT_SUCCESS;
}
} // namespace ants
} // namespace itk
#endif // itkantsReadWriteTransform_h
