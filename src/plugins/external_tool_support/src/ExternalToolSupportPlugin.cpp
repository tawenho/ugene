/**
 * UGENE - Integrated Bioinformatics Tools.
 * Copyright (C) 2008-2021 UniPro <ugene@unipro.ru>
 * http://ugene.net
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 */

#include "ExternalToolSupportPlugin.h"
#include <spades/SpadesSupport.h>

#include "samtools/BcfToolsSupport.h"
#include "samtools/SamToolsExtToolSupport.h"
#include "samtools/TabixSupport.h"

#include <U2Algorithm/CDSearchTaskFactoryRegistry.h>
#include <U2Algorithm/DnaAssemblyAlgRegistry.h>

#include <U2Core/AppContext.h>
#include <U2Core/DNAAlphabet.h>
#include <U2Core/DataBaseRegistry.h>
#include <U2Core/ExternalToolRegistry.h>
#include <U2Core/GAutoDeleteList.h>
#include <U2Core/MultiTask.h>
#include <U2Core/ScriptingToolRegistry.h>
#include <U2Core/U2SafePoints.h>

#include <U2Gui/ToolsMenu.h>

#include <U2Test/GTestFrameworkComponents.h>
#include <U2Test/XMLTestFormat.h>

#include <U2View/ADVSequenceObjectContext.h>
#include <U2View/MaEditorFactory.h>

#include "ETSProjectViewItemsContoller.h"
#include "ExternalToolSupportSettings.h"
#include "ExternalToolSupportSettingsController.h"
#include "R/RSupport.h"
#include "bedtools/BedToolsWorkersLibrary.h"
#include "bedtools/BedtoolsSupport.h"
#include "bigWigTools/BedGraphToBigWigWorker.h"
#include "bigWigTools/BigWigSupport.h"
#include "blast_plus/AlignToReferenceBlastWorker.h"
#include "blast_plus/BlastDBCmdSupport.h"
#include "blast_plus/BlastPlusSupport.h"
#include "blast_plus/BlastPlusWorker.h"
#include "blast_plus/FormatDBSupport.h"
#include "blast_plus/RPSBlastSupportTask.h"
#include "bowtie/BowtieSettingsWidget.h"
#include "bowtie/BowtieSupport.h"
#include "bowtie/BowtieTask.h"
#include "bowtie/BowtieWorker.h"
#include "bowtie/bowtie_tests/bowtieTests.h"
#include "bowtie2/Bowtie2SettingsWidget.h"
#include "bowtie2/Bowtie2Support.h"
#include "bowtie2/Bowtie2Task.h"
#include "bowtie2/Bowtie2Worker.h"
#include "bowtie2/bowtie2_tests/Bowtie2Tests.h"
#include "bwa/BwaMemWorker.h"
#include "bwa/BwaSettingsWidget.h"
#include "bwa/BwaSupport.h"
#include "bwa/BwaTask.h"
#include "bwa/BwaWorker.h"
#include "bwa/bwa_tests/bwaTests.h"
#include "cap3/CAP3Support.h"
#include "cap3/CAP3Worker.h"
#include "ceas/CEASReportWorker.h"
#include "ceas/CEASSupport.h"
#include "clustalo/ClustalOSupport.h"
#include "clustalo/ClustalOWorker.h"
#include "clustalw/ClustalWSupport.h"
#include "clustalw/ClustalWWorker.h"
#include "conduct_go/ConductGOSupport.h"
#include "conduct_go/ConductGOWorker.h"
#include "conservation_plot/ConservationPlotSupport.h"
#include "conservation_plot/ConservationPlotWorker.h"
#include "cufflinks/CuffdiffWorker.h"
#include "cufflinks/CufflinksSupport.h"
#include "cufflinks/CufflinksWorker.h"
#include "cufflinks/CuffmergeWorker.h"
#include "cufflinks/GffreadWorker.h"
#include "cutadapt/CutadaptSupport.h"
#include "cutadapt/CutadaptWorker.h"
#include "fastqc/FastqcSupport.h"
#include "fastqc/FastqcWorker.h"
#include "hmmer/HmmerBuildWorker.h"
#include "hmmer/HmmerSearchWorker.h"
#include "hmmer/HmmerSupport.h"
#include "hmmer/HmmerTests.h"
#include "java/JavaSupport.h"
#include "macs/MACSSupport.h"
#include "macs/MACSWorker.h"
#include "mafft/MAFFTSupport.h"
#include "mafft/MAFFTWorker.h"
#include "mrbayes/MrBayesSupport.h"
#include "mrbayes/MrBayesTests.h"
#include "peak2gene/Peak2GeneSupport.h"
#include "peak2gene/Peak2GeneWorker.h"
#include "perl/PerlSupport.h"
#include "phyml/PhyMLSupport.h"
#include "phyml/PhyMLTests.h"
#include "python/PythonSupport.h"
#include "seqpos/SeqPosSupport.h"
#include "seqpos/SeqPosWorker.h"
#include "snpeff/SnpEffSupport.h"
#include "snpeff/SnpEffWorker.h"
#include "spades/SpadesWorker.h"
#include "spidey/SpideySupport.h"
#include "spidey/SpideySupportTask.h"
#include "stringtie/StringTieSupport.h"
#include "stringtie/StringTieWorker.h"
#include "stringtie/StringtieGeneAbundanceReportWorkerFactory.h"
#include "tcoffee/TCoffeeSupport.h"
#include "tcoffee/TCoffeeWorker.h"
#include "tophat/TopHatSupport.h"
#include "tophat/TopHatWorker.h"
#include "trimmomatic/TrimmomaticSupport.h"
#include "trimmomatic/TrimmomaticWorkerFactory.h"
#include "utils/ExternalToolSupportAction.h"
#include "utils/ExternalToolValidateTask.h"
#include "vcftools/VcfConsensusSupport.h"
#include "vcftools/VcfConsensusWorker.h"
#include "vcfutils/VcfutilsSupport.h"

#define TOOLS "tools"

namespace U2 {

extern "C" Q_DECL_EXPORT Plugin *U2_PLUGIN_INIT_FUNC() {
    return new ExternalToolSupportPlugin();
}

ExternalToolSupportPlugin::ExternalToolSupportPlugin()
    : Plugin(tr("External tool support"), tr("Runs other external tools")) {
    // External tools serialize additional tool info into QSettings and using StrStrMap type.
    qRegisterMetaTypeStreamOperators<StrStrMap>("StrStrMap");

    //External tools registry keeps order of items added
    //it is important because there might be dependencies
    ExternalToolRegistry *etRegistry = AppContext::getExternalToolRegistry();
    SAFE_POINT(etRegistry != nullptr, "ExternalToolRegistry is null", );

    // python with modules
    etRegistry->registerEntry(new PythonSupport());
    etRegistry->registerEntry(new PythonModuleDjangoSupport());
    etRegistry->registerEntry(new PythonModuleNumpySupport());
    etRegistry->registerEntry(new PythonModuleBioSupport());

    // Rscript with modules
    etRegistry->registerEntry(new RSupport());
    etRegistry->registerEntry(new RModuleGostatsSupport());
    etRegistry->registerEntry(new RModuleGodbSupport());
    etRegistry->registerEntry(new RModuleHgu133adbSupport());
    etRegistry->registerEntry(new RModuleHgu133bdbSupport());
    etRegistry->registerEntry(new RModuleHgu133plus2dbSupport());
    etRegistry->registerEntry(new RModuleHgu95av2dbSupport());
    etRegistry->registerEntry(new RModuleMouse430a2dbSupport());
    etRegistry->registerEntry(new RModuleCelegansdbSupport());
    etRegistry->registerEntry(new RModuleDrosophila2dbSupport());
    etRegistry->registerEntry(new RModuleOrghsegdbSupport());
    etRegistry->registerEntry(new RModuleOrgmmegdbSupport());
    etRegistry->registerEntry(new RModuleOrgceegdbSupport());
    etRegistry->registerEntry(new RModuleOrgdmegdbSupport());
    etRegistry->registerEntry(new RModuleSeqlogoSupport());

    //perl
    etRegistry->registerEntry(new PerlSupport());

    //java
    etRegistry->registerEntry(new JavaSupport());

    //ClustalW
    ClustalWSupport *clustalWTool = new ClustalWSupport();
    etRegistry->registerEntry(clustalWTool);

    //ClustalO
    ClustalOSupport *clustalOTool = new ClustalOSupport();
    etRegistry->registerEntry(clustalOTool);

    //MAFFT
    MAFFTSupport *mAFFTTool = new MAFFTSupport();
    etRegistry->registerEntry(mAFFTTool);

    //T-Coffee
    TCoffeeSupport *tCoffeeTool = new TCoffeeSupport();
    etRegistry->registerEntry(tCoffeeTool);

    //MrBayes
    etRegistry->registerEntry(new MrBayesSupport());

    //PhyML
    etRegistry->registerEntry(new PhyMLSupport());

    if (AppContext::getMainWindow()) {
        clustalWTool->getViewContext()->setParent(this);
        clustalWTool->getViewContext()->init();

        ExternalToolSupportAction *clustalWAction = new ExternalToolSupportAction(tr("Align with ClustalW..."), this, QStringList(ClustalWSupport::ET_CLUSTAL_ID));
        clustalWAction->setObjectName(ToolsMenu::MALIGN_CLUSTALW);
        connect(clustalWAction, SIGNAL(triggered()), clustalWTool, SLOT(sl_runWithExtFileSpecify()));
        ToolsMenu::addAction(ToolsMenu::MALIGN_MENU, clustalWAction);

        clustalOTool->getViewContext()->setParent(this);
        clustalOTool->getViewContext()->init();

        ExternalToolSupportAction *clustalOAction = new ExternalToolSupportAction(tr("Align with ClustalO..."), this, QStringList(ClustalOSupport::ET_CLUSTALO_ID));
        clustalOAction->setObjectName(ToolsMenu::MALIGN_CLUSTALO);
        connect(clustalOAction, SIGNAL(triggered()), clustalOTool, SLOT(sl_runWithExtFileSpecify()));
        ToolsMenu::addAction(ToolsMenu::MALIGN_MENU, clustalOAction);

        mAFFTTool->getViewContext()->setParent(this);
        mAFFTTool->getViewContext()->init();

        ExternalToolSupportAction *mAFFTAction = new ExternalToolSupportAction(tr("Align with MAFFT..."), this, QStringList(MAFFTSupport::ET_MAFFT_ID));
        mAFFTAction->setObjectName(ToolsMenu::MALIGN_MAFFT);
        connect(mAFFTAction, SIGNAL(triggered()), mAFFTTool, SLOT(sl_runWithExtFileSpecify()));
        ToolsMenu::addAction(ToolsMenu::MALIGN_MENU, mAFFTAction);

        tCoffeeTool->getViewContext()->setParent(this);
        tCoffeeTool->getViewContext()->init();

        ExternalToolSupportAction *tCoffeeAction = new ExternalToolSupportAction(tr("Align with T-Coffee..."), this, QStringList(TCoffeeSupport::ET_TCOFFEE_ID));
        tCoffeeAction->setObjectName(ToolsMenu::MALIGN_TCOFFEE);
        connect(tCoffeeAction, SIGNAL(triggered()), tCoffeeTool, SLOT(sl_runWithExtFileSpecify()));
        ToolsMenu::addAction(ToolsMenu::MALIGN_MENU, tCoffeeAction);
    }

    //MakeBLASTDB from BLAST+
    FormatDBSupport *makeBLASTDBTool = new FormatDBSupport();
    etRegistry->registerEntry(makeBLASTDBTool);

    //BlastAll
    BlastPlusSupport *blastNPlusTool = new BlastPlusSupport(BlastPlusSupport::ET_BLASTN_ID, BlastPlusSupport::ET_BLASTN);
    etRegistry->registerEntry(blastNPlusTool);
    BlastPlusSupport *blastPPlusTool = new BlastPlusSupport(BlastPlusSupport::ET_BLASTP_ID, BlastPlusSupport::ET_BLASTP);
    etRegistry->registerEntry(blastPPlusTool);
    BlastPlusSupport *blastXPlusTool = new BlastPlusSupport(BlastPlusSupport::ET_BLASTX_ID, BlastPlusSupport::ET_BLASTX);
    etRegistry->registerEntry(blastXPlusTool);
    BlastPlusSupport *tBlastNPlusTool = new BlastPlusSupport(BlastPlusSupport::ET_TBLASTN_ID, BlastPlusSupport::ET_TBLASTN);
    etRegistry->registerEntry(tBlastNPlusTool);
    BlastPlusSupport *tBlastXPlusTool = new BlastPlusSupport(BlastPlusSupport::ET_TBLASTX_ID, BlastPlusSupport::ET_TBLASTX);
    etRegistry->registerEntry(tBlastXPlusTool);
    BlastPlusSupport *rpsblastTool = new BlastPlusSupport(BlastPlusSupport::ET_RPSBLAST_ID, BlastPlusSupport::ET_RPSBLAST);
    etRegistry->registerEntry(rpsblastTool);
    BlastDbCmdSupport *blastDbCmdSupport = new BlastDbCmdSupport();
    etRegistry->registerEntry(blastDbCmdSupport);

    // CAP3
    CAP3Support *cap3Tool = new CAP3Support(CAP3Support::ET_CAP3_ID, CAP3Support::ET_CAP3);
    etRegistry->registerEntry(cap3Tool);

    // Bowtie 1
    etRegistry->registerEntry(new BowtieSupport(BowtieSupport::ET_BOWTIE_ID));
    etRegistry->registerEntry(new BowtieSupport(BowtieSupport::ET_BOWTIE_BUILD_ID));

    // Bowtie 2
    etRegistry->registerEntry(new Bowtie2Support(Bowtie2Support::ET_BOWTIE2_ALIGN_ID));
    etRegistry->registerEntry(new Bowtie2Support(Bowtie2Support::ET_BOWTIE2_BUILD_ID));
    etRegistry->registerEntry(new Bowtie2Support(Bowtie2Support::ET_BOWTIE2_INSPECT_ID));

    // BWA
    etRegistry->registerEntry(new BwaSupport());

    // SPAdes
    SpadesSupport::checkIn();

    // SAMtools (external tool)
    etRegistry->registerEntry(new SamToolsExtToolSupport());

    // BCFtools (external tool)
    etRegistry->registerEntry(new BcfToolsSupport());

    // Tabix
    etRegistry->registerEntry(new TabixSupport());

    // VcfConsensus
    etRegistry->registerEntry(new VcfConsensusSupport());

    // Spidey
    SpideySupport *spideySupport = new SpideySupport();
    etRegistry->registerEntry(spideySupport);

    //bedtools
    etRegistry->registerEntry(new BedtoolsSupport());

    //cutadapt
    etRegistry->registerEntry(new CutadaptSupport());

    //bigwig
    etRegistry->registerEntry(new BigWigSupport());

    // TopHat
    etRegistry->registerEntry(new TopHatSupport());

    // Cufflinks external tools
    etRegistry->registerEntry(new CufflinksSupport(CufflinksSupport::ET_CUFFCOMPARE_ID, CufflinksSupport::ET_CUFFCOMPARE));
    etRegistry->registerEntry(new CufflinksSupport(CufflinksSupport::ET_CUFFDIFF_ID, CufflinksSupport::ET_CUFFDIFF));
    etRegistry->registerEntry(new CufflinksSupport(CufflinksSupport::ET_CUFFLINKS_ID, CufflinksSupport::ET_CUFFLINKS));
    etRegistry->registerEntry(new CufflinksSupport(CufflinksSupport::ET_CUFFMERGE_ID, CufflinksSupport::ET_CUFFMERGE));
    etRegistry->registerEntry(new CufflinksSupport(CufflinksSupport::ET_GFFREAD_ID, CufflinksSupport::ET_GFFREAD));

    // CEAS
    etRegistry->registerEntry(new CEASSupport());

    // MACS
    etRegistry->registerEntry(new MACSSupport());

    // peak2gene
    etRegistry->registerEntry(new Peak2GeneSupport());

    //ConservationPlot
    etRegistry->registerEntry(new ConservationPlotSupport());

    //SeqPos
    etRegistry->registerEntry(new SeqPosSupport());

    //ConductGO
    etRegistry->registerEntry(new ConductGOSupport());

    //Vcfutils
    etRegistry->registerEntry(new VcfutilsSupport());

    //SnpEff
    etRegistry->registerEntry(new SnpEffSupport());

    //FastQC
    etRegistry->registerEntry(new FastQCSupport());

    // StringTie
    etRegistry->registerEntry(new StringTieSupport());

    //HMMER
    etRegistry->registerEntry(new HmmerSupport(HmmerSupport::BUILD_TOOL_ID, HmmerSupport::BUILD_TOOL));
    etRegistry->registerEntry(new HmmerSupport(HmmerSupport::SEARCH_TOOL_ID, HmmerSupport::SEARCH_TOOL));
    etRegistry->registerEntry(new HmmerSupport(HmmerSupport::PHMMER_TOOL_ID, HmmerSupport::PHMMER_TOOL));

    //Trimmomatic
    etRegistry->registerEntry(new TrimmomaticSupport());

    if (AppContext::getMainWindow() != nullptr) {
        etRegistry->setToolkitDescription("BLAST", tr("The <i>Basic Local Alignment Search Tool</i> (BLAST) finds regions of local similarity between sequences. "
                                                      "The program compares nucleotide or protein sequences to sequence databases and calculates the statistical significance of matches. "
                                                      "BLAST can be used to infer functional and evolutionary relationships between sequences as well as help identify members of gene families."));

        etRegistry->setToolkitDescription("BLAST+", tr("<i>BLAST+</i> is a new version of the BLAST package from the NCBI."));

        etRegistry->setToolkitDescription("GPU-BLAST+", tr("<i>BLAST+</i> is a new version of the BLAST package from the NCBI."));

        etRegistry->setToolkitDescription("Bowtie", tr("<i>Bowtie<i> is an ultrafast, memory-efficient short read aligner. "
                                                       "It aligns short DNA sequences (reads) to the human genome at "
                                                       "a rate of over 25 million 35-bp reads per hour. "
                                                       "Bowtie indexes the genome with a Burrows-Wheeler index to keep "
                                                       "its memory footprint small: typically about 2.2 GB for the human "
                                                       "genome (2.9 GB for paired-end). <a href='http://qt-project.org/doc/qt-4.8/qtextbrowser.html#anchorClicked'>Link text</a> "));

        etRegistry->setToolkitDescription("Cufflinks", tr("<i>Cufflinks</i> assembles transcripts, estimates"
                                                          " their abundances, and tests for differential expression and regulation"
                                                          " in RNA-Seq samples. It accepts aligned RNA-Seq reads and assembles"
                                                          " the alignments into a parsimonious set of transcripts. It also estimates"
                                                          " the relative abundances of these transcripts based on how many reads"
                                                          " support each one, taking into account biases in library preparation protocols. "));

        etRegistry->setToolkitDescription("Bowtie2", tr("<i>Bowtie 2</i> is an ultrafast and memory-efficient tool"
                                                        " for aligning sequencing reads to long reference sequences. It is particularly good"
                                                        " at aligning reads of about 50 up to 100s or 1000s of characters, and particularly"
                                                        " good at aligning to relatively long (e.g. mammalian) genomes."
                                                        " <br/><br/>It indexes the genome with an FM index to keep its memory footprint small:"
                                                        " for the human genome, its memory footprint is typically around 3.2Gb."
                                                        " <br/><br/><i>Bowtie 2</i> supports gapped, local, and paired-end alignment modes."));

        etRegistry->setToolkitDescription("Cistrome", tr("<i>Cistrome</i> is a UGENE version of Cistrome pipeline which also includes some tools useful for ChIP-seq analysis"
                                                         "This pipeline is aimed to provide the following analysis steps: peak calling and annotating, motif search and gene ontology."));

        ExternalToolSupportAction *makeBLASTDBAction = new ExternalToolSupportAction(tr("BLAST+ make database..."), this, QStringList(FormatDBSupport::ET_MAKEBLASTDB_ID));
        makeBLASTDBAction->setObjectName(ToolsMenu::BLAST_DBP);
        connect(makeBLASTDBAction, SIGNAL(triggered()), makeBLASTDBTool, SLOT(sl_runWithExtFileSpecify()));

        ExternalToolSupportAction *alignToRefBlastAction = new ExternalToolSupportAction(tr("Map reads to reference..."),
                                                                                         this,
                                                                                         QStringList() << FormatDBSupport::ET_MAKEBLASTDB_ID << BlastPlusSupport::ET_BLASTN_ID);
        alignToRefBlastAction->setObjectName(ToolsMenu::SANGER_ALIGN);
        connect(alignToRefBlastAction, SIGNAL(triggered(bool)), blastNPlusTool, SLOT(sl_runAlign()));

        BlastPlusSupportContext *blastPlusViewCtx = new BlastPlusSupportContext(this);
        blastPlusViewCtx->setParent(this);    //may be problems???
        blastPlusViewCtx->init();
        QStringList toolList;
        toolList << BlastPlusSupport::ET_BLASTN_ID << BlastPlusSupport::ET_BLASTP_ID << BlastPlusSupport::ET_BLASTX_ID << BlastPlusSupport::ET_TBLASTN_ID << BlastPlusSupport::ET_TBLASTX_ID << BlastPlusSupport::ET_RPSBLAST_ID;
        ExternalToolSupportAction *blastPlusAction = new ExternalToolSupportAction(tr("BLAST+ search..."), this, toolList);
        blastPlusAction->setObjectName(ToolsMenu::BLAST_SEARCHP);
        connect(blastPlusAction, SIGNAL(triggered()), blastNPlusTool, SLOT(sl_runWithExtFileSpecify()));

        ExternalToolSupportAction *blastPlusCmdAction = new ExternalToolSupportAction(tr("BLAST+ query database..."), this, QStringList(BlastDbCmdSupport::ET_BLASTDBCMD_ID));
        blastPlusCmdAction->setObjectName(ToolsMenu::BLAST_QUERYP);
        connect(blastPlusCmdAction, SIGNAL(triggered()), blastDbCmdSupport, SLOT(sl_runWithExtFileSpecify()));

        //Add to menu NCBI Toolkit
        ToolsMenu::addAction(ToolsMenu::BLAST_MENU, makeBLASTDBAction);
        ToolsMenu::addAction(ToolsMenu::BLAST_MENU, blastPlusAction);
        ToolsMenu::addAction(ToolsMenu::BLAST_MENU, blastPlusCmdAction);

        ExternalToolSupportAction *cap3Action = new ExternalToolSupportAction(QString(tr("Reads de novo assembly (with %1)...")).arg(cap3Tool->getName()), this, QStringList(cap3Tool->getId()));
        cap3Action->setObjectName(ToolsMenu::SANGER_DENOVO);
        connect(cap3Action, SIGNAL(triggered()), cap3Tool, SLOT(sl_runWithExtFileSpecify()));
        ToolsMenu::addAction(ToolsMenu::SANGER_MENU, cap3Action);
        ToolsMenu::addAction(ToolsMenu::SANGER_MENU, alignToRefBlastAction);

        GObjectViewWindowContext *spideyCtx = spideySupport->getViewContext();
        spideyCtx->setParent(this);
        spideyCtx->init();

        HmmerContext *hmmerContext = new HmmerContext(this);
        hmmerContext->init();
    }

    AppContext::getCDSFactoryRegistry()->registerFactory(new CDSearchLocalTaskFactory(), CDSearchFactoryRegistry::LocalSearch);

    QStringList referenceFormats(BaseDocumentFormats::FASTA);
    QStringList readsFormats;
    readsFormats << BaseDocumentFormats::FASTA;
    readsFormats << BaseDocumentFormats::FASTQ;

    DnaAssemblyAlgRegistry *dnaAssemblyRegistry = AppContext::getDnaAssemblyAlgRegistry();
    dnaAssemblyRegistry->registerAlgorithm(new DnaAssemblyAlgorithmEnv(BowtieTask::taskName, new BowtieTaskFactory(), new BowtieGUIExtensionsFactory(), true /*Index*/, false /*Dbi*/, true /*Paired-reads*/, referenceFormats, readsFormats));
    dnaAssemblyRegistry->registerAlgorithm(new DnaAssemblyAlgorithmEnv(BwaTask::ALGORITHM_BWA_ALN, new BwaTaskFactory(), new BwaGUIExtensionsFactory(), true /*Index*/, false /*Dbi*/, true /*Paired*/, referenceFormats, readsFormats));
    dnaAssemblyRegistry->registerAlgorithm(new DnaAssemblyAlgorithmEnv(BwaTask::ALGORITHM_BWA_SW, new BwaTaskFactory(), new BwaSwGUIExtensionsFactory(), true /*Index*/, false /*Dbi*/, false /*Paired*/, referenceFormats, readsFormats));
    dnaAssemblyRegistry->registerAlgorithm(new DnaAssemblyAlgorithmEnv(BwaTask::ALGORITHM_BWA_MEM, new BwaTaskFactory(), new BwaMemGUIExtensionsFactory(), true /*Index*/, false /*Dbi*/, true /*Paired*/, referenceFormats, readsFormats));

    readsFormats << BaseDocumentFormats::RAW_DNA_SEQUENCE;
    dnaAssemblyRegistry->registerAlgorithm(new DnaAssemblyAlgorithmEnv(Bowtie2Task::taskName, new Bowtie2TaskFactory(), new Bowtie2GUIExtensionsFactory(), true /*Index*/, false /*Dbi*/, true /*Paired-reads*/, referenceFormats, readsFormats));

    GTestFormatRegistry *testFormatRegistry = AppContext::getTestFramework()->getTestFormatRegistry();
    XMLTestFormat *xmlTestFormat = qobject_cast<XMLTestFormat *>(testFormatRegistry->findFormat("XML"));
    xmlTestFormat->registerTestFactories(BowtieTests::createTestFactories());
    xmlTestFormat->registerTestFactories(Bowtie2Tests::createTestFactories());
    xmlTestFormat->registerTestFactories(BwaTests::createTestFactories());
    xmlTestFormat->registerTestFactories(MrBayesToolTests::createTestFactories());
    xmlTestFormat->registerTestFactories(PhyMLToolTests::createTestFactories());
    xmlTestFormat->registerTestFactories(HmmerTests::createTestFactories());

    auto externalToolManager = new ExternalToolManagerImpl();
    externalToolManager->setParent(this);
    etRegistry->setManager(externalToolManager);

    registerSettingsController();

    registerWorkers();

    if (AppContext::getMainWindow()) {
        services << new ExternalToolSupportService();    // Add project view service
    }
}

ExternalToolSupportPlugin::~ExternalToolSupportPlugin() {
    ExternalToolSupportSettings::saveExternalToolsToAppConfig();
}

void ExternalToolSupportPlugin::registerSettingsController() {
    if (AppContext::getMainWindow() != nullptr) {
        AppContext::getAppSettingsGUI()->registerPage(new ExternalToolSupportSettingsPageController());
    }
}

void ExternalToolSupportPlugin::registerWorkers() {
    LocalWorkflow::ClustalWWorkerFactory::init();
    LocalWorkflow::ClustalOWorkerFactory::init();
    LocalWorkflow::MAFFTWorkerFactory::init();

    LocalWorkflow::AlignToReferenceBlastWorkerFactory::init();
    LocalWorkflow::BlastPlusWorkerFactory::init();

    LocalWorkflow::TCoffeeWorkerFactory::init();
    LocalWorkflow::CuffdiffWorkerFactory::init();
    LocalWorkflow::CufflinksWorkerFactory::init();
    LocalWorkflow::CuffmergeWorkerFactory::init();
    LocalWorkflow::GffreadWorkerFactory::init();
    LocalWorkflow::TopHatWorkerFactory::init();
    LocalWorkflow::CEASReportWorkerFactory::init();
    LocalWorkflow::MACSWorkerFactory::init();
    LocalWorkflow::Peak2GeneWorkerFactory::init();
    LocalWorkflow::ConservationPlotWorkerFactory::init();
    LocalWorkflow::SeqPosWorkerFactory::init();
    LocalWorkflow::ConductGOWorkerFactory::init();
    LocalWorkflow::CAP3WorkerFactory::init();
    LocalWorkflow::VcfConsensusWorkerFactory::init();
    LocalWorkflow::BwaMemWorkerFactory::init();
    LocalWorkflow::BwaWorkerFactory::init();
    LocalWorkflow::BowtieWorkerFactory::init();
    LocalWorkflow::Bowtie2WorkerFactory::init();
    LocalWorkflow::SlopbedWorkerFactory::init();
    LocalWorkflow::GenomecovWorkerFactory::init();
    LocalWorkflow::BedGraphToBigWigFactory::init();

    LocalWorkflow::SnpEffFactory::init();
    LocalWorkflow::FastQCFactory::init();
    LocalWorkflow::CutAdaptFastqWorkerFactory::init();
    LocalWorkflow::TrimmomaticWorkerFactory::init();
    LocalWorkflow::BedtoolsIntersectWorkerFactory::init();
    LocalWorkflow::HmmerBuildWorkerFactory::init();
    LocalWorkflow::HmmerSearchWorkerFactory::init();
    LocalWorkflow::StringTieWorkerFactory::init();
    LocalWorkflow::StringtieGeneAbundanceReportWorkerFactory::init();
}

//////////////////////////////////////////////////////////////////////////
// Service
ExternalToolSupportService::ExternalToolSupportService()
    : Service(Service_ExternalToolSupport, tr("External tools support"), tr("Provides support to run external tools from UGENE"), QList<ServiceType>() << Service_ProjectView) {
    projectViewController = NULL;
}

void ExternalToolSupportService::serviceStateChangedCallback(ServiceState oldState, bool enabledStateChanged) {
    Q_UNUSED(oldState);

    if (!enabledStateChanged) {
        return;
    }
    if (isEnabled()) {
        projectViewController = new ETSProjectViewItemsContoller(this);
    } else {
        delete projectViewController;
        projectViewController = NULL;
    }
}

}    // namespace U2
